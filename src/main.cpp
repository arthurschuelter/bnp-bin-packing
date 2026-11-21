//----------------------------------------------------------------------
// Main program for the Bin Packing Problem with Conflict Classes.
//
// Usage: binpack -f <input file> -o <output file> -t <seconds> [-r <seed>]
//
// It reads the instance, stores everything in a BinPackingData object, calls
// SolveBinPacking() (file binpacksolver.cpp), checks that the solution found
// is feasible and writes it to the output file. The formats of the files are
// described in the project statement.
//----------------------------------------------------------------------
#include "binpacking.h"
#include "binpacksolver.h"
#include "commandline.h"
#include <cerrno>
#include <climits>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

// Reads all the tokens of the file, ignoring the lines that start with '#'.
static bool ReadTokens(const string &filename, vector<string> &tokens)
{
	ifstream in(filename.c_str());
	if (!in)
		return false;
	string line;
	while (getline(in, line))
	{
		size_t p = line.find_first_not_of(" \t\r");
		if (p == string::npos || line[p] == '#')
			continue;
		istringstream ss(line);
		string tok;
		while (ss >> tok)
			tokens.push_back(tok);
	}
	return true;
}

// Input format (lines starting with '#' are comments):
//   n C k
//   s_1 s_2 ... s_n
//   k lines, one per class:  m_t i_1 ... i_{m_t}
static bool ReadInstance(const string &filename, BinPackingData &data, string &err)
{
	vector<string> tok;
	if (!ReadTokens(filename, tok))
	{
		err = "cannot open " + filename;
		return false;
	}
	size_t pos = 0;
	long long v;
	auto next = [&](long long &x) -> bool
	{
		if (pos >= tok.size())
		{
			err = "unexpected end of file";
			return false;
		}
		char *end;
		errno = 0;
		x = strtoll(tok[pos].c_str(), &end, 10);
		if (errno == ERANGE || end == tok[pos].c_str() || *end != '\0')
		{
			err = "invalid number: " + tok[pos];
			return false;
		}
		pos++;
		return true;
	};
	if (!next(v))
		return false;
	if (v < 1 || v >= INT_MAX)
	{
		err = "invalid n";
		return false;
	}
	data.n = (int)v;
	if (!next(v))
		return false;
	data.C = v;
	if (!next(v))
		return false;
	if (v < 0 || v > INT_MAX)
	{
		err = "invalid k";
		return false;
	}
	data.k = (int)v;
	if (data.n < 1 || data.C < 1 || data.k < 0)
	{
		err = "invalid n, C or k";
		return false;
	}
	data.size.assign(data.n + 1, 0);
	for (int i = 1; i <= data.n; i++)
	{
		if (!next(v))
			return false;
		if (v < 1 || v > data.C)
		{
			err = "size of an item is not in 1..C";
			return false;
		}
		data.size[i] = v;
	}
	data.cls.assign(data.k, vector<int>());
	for (int t = 0; t < data.k; t++)
	{
		if (!next(v))
			return false;
		long long m = v;
		if (m < 0)
		{
			err = "invalid size of a class";
			return false;
		}
		set<int> items; // removes repeated items of a class
		for (long long a = 0; a < m; a++)
		{
			if (!next(v))
				return false;
			if (v < 1 || v > data.n)
			{
				err = "item out of range in a class";
				return false;
			}
			items.insert((int)v);
		}
		data.cls[t].assign(items.begin(), items.end());
	}
	if (pos != tok.size())
		cout << "Warning: extra data at the end of the input file" << endl;
	data.Prepare();
	return true;
}

// Output format (lines starting with '#' are comments):
//   value of the best solution (number of bins)
//   best lower bound
//   number of bins
//   one line per bin with the items it contains
static bool WriteSolution(const string &filename, const BinPackingData &data)
{
	ofstream out(filename.c_str());
	if (!out)
		return false;
	out << "# valor da solucao (numero de recipientes)\n"
	    << data.best_value << "\n"
	    << "# limitante inferior\n"
	    << data.lower_bound << "\n"
	    << "# numero de recipientes\n"
	    << data.best_solution.size() << "\n"
	    << "# itens de cada recipiente\n";
	for (size_t b = 0; b < data.best_solution.size(); b++)
	{
		for (size_t a = 0; a < data.best_solution[b].size(); a++)
			out << (a ? " " : "") << data.best_solution[b][a];
		out << "\n";
	}
	return true;
}

// Prints every bin with its load (sum of the item sizes) against the capacity
// C, followed by a summary of the occupation and the bounds of the run.
static void LogResults(const BinPackingData &data)
{
	const size_t bins = data.best_solution.size();
	const int width = static_cast<int>(to_string(data.C).size());
	const int index_width = static_cast<int>(to_string(bins).size());
	// long double: the total may exceed LLONG_MAX when C is close to it.
	long double total = 0;
	long long min_load = data.C, max_load = 0;
	size_t full = 0;

	cout << "[Solver] Bins (load/C, occupation, items):\n";
	for (size_t b = 0; b < bins; b++)
	{
		long long load = 0;
		for (int i : data.best_solution[b])
			load += data.size[i];
		total += load;
		min_load = min(min_load, load);
		max_load = max(max_load, load);
		if (load == data.C)
			++full;

		cout << "  " << setw(index_width) << b << ": " << setw(width) << load << "/" << data.C
		     << " (" << fixed << setprecision(1) << setw(5) << 100.0L * load / data.C << "%) [";
		for (size_t a = 0; a < data.best_solution[b].size(); a++)
			cout << (a ? ", " : "") << data.best_solution[b][a];
		cout << "]\n";
	}
	if (bins == 0)
		min_load = 0;

	const long double capacity = static_cast<long double>(data.C) * bins;
	cout << "[Solver] Feasible solution with " << data.best_value << " bins\n"
	     << "\t Total load: " << setprecision(0) << total << " of " << bins << " x " << data.C
	     << " (" << setprecision(1) << (bins ? 100.0L * total / capacity : 0.0L)
	     << "% occupation)\n"
	     << "\t Load per bin: min=" << min_load << ", max=" << max_load
	     << ", avg=" << setprecision(1) << (bins ? total / bins : 0.0L) << "; full bins: " << full
	     << "\n"
	     << "\t LB=" << data.lower_bound << "; UB=" << data.best_value
	     << (data.best_value <= data.lower_bound ? " (optimality proven)"
	                                             : " (feasible, optimality not proven)")
	     << "\n"
	     << "\t Total time: " << setprecision(3) << data.elapsed << " s" << endl;
	cout.unsetf(ios::floatfield);
	cout << setprecision(6);
}

int main(int argc, char *argv[])
{
	const auto options = ParseCommandLine(argc, argv);
	if (!options)
		return 1;

	BinPackingData data;
	string err;
	if (!ReadInstance(options->input_file, data, err))
	{
		cout << "Error reading " << options->input_file << ": " << err << endl;
		return 1;
	}
	data.seed = options->seed;
	cout << "Instance: n = " << data.n << ", C = " << data.C << ", k = " << data.k
	     << " conflict classes; lower bound " << data.lower_bound << endl;

	SolveBinPacking(data, options->time_limit);

	if (!data.HasSolution())
	{
		cout << "Error: the solver did not return any solution" << endl;
		return 1;
	}
	if (!data.CheckSolution(data.best_solution, err))
	{
		cout << "Error: the solution is NOT feasible: " << err << endl;
		return 1;
	}
	if (data.best_value != (int)data.best_solution.size())
	{
		cout << "Error: best_value does not match the number of bins" << endl;
		return 1;
	}

	LogResults(data);

	if (!WriteSolution(options->output_file, data))
	{
		cout << "Error: cannot write " << options->output_file << endl;
		return 1;
	}
	return 0;
}
