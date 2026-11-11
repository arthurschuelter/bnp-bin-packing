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
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include "binpacking.h"
#include "binpacksolver.h"

using namespace std;

static void Usage(const char *prog) {
  cout << "Usage: " << prog << " -f <input file> -o <output file> -t <seconds> [-r <seed>]" << endl
       << "  -f  input file with the instance" << endl
       << "  -o  output file where the best solution is written" << endl
       << "  -t  maximum running time T, in seconds" << endl
       << "  -r  seed of the random number generator (default 1)" << endl;
}

// Reads all the tokens of the file, ignoring the lines that start with '#'.
static bool ReadTokens(const string &filename, vector<string> &tokens) {
  ifstream in(filename.c_str());
  if (!in) return false;
  string line;
  while (getline(in, line)) {
    size_t p = line.find_first_not_of(" \t\r");
    if (p == string::npos || line[p] == '#') continue;
    istringstream ss(line);
    string tok;
    while (ss >> tok) tokens.push_back(tok);
  }
  return true;
}

// Input format (lines starting with '#' are comments):
//   n C k
//   s_1 s_2 ... s_n
//   k lines, one per class:  m_t i_1 ... i_{m_t}
static bool ReadInstance(const string &filename, BinPackingData &data, string &err) {
  vector<string> tok;
  if (!ReadTokens(filename, tok)) { err = "cannot open " + filename; return false; }
  size_t pos = 0;
  long long v;
  auto next = [&](long long &x) -> bool {
    if (pos >= tok.size()) { err = "unexpected end of file"; return false; }
    char *end;
    x = strtoll(tok[pos].c_str(), &end, 10);
    if (*end != '\0') { err = "invalid number: " + tok[pos]; return false; }
    pos++;
    return true;
  };
  if (!next(v)) return false;  data.n = (int)v;
  if (!next(v)) return false;  data.C = v;
  if (!next(v)) return false;  data.k = (int)v;
  if (data.n < 1 || data.C < 1 || data.k < 0) { err = "invalid n, C or k"; return false; }
  data.size.assign(data.n + 1, 0);
  for (int i = 1; i <= data.n; i++) {
    if (!next(v)) return false;
    if (v < 1 || v > data.C) { err = "size of an item is not in 1..C"; return false; }
    data.size[i] = v;
  }
  data.cls.assign(data.k, vector<int>());
  for (int t = 0; t < data.k; t++) {
    if (!next(v)) return false;
    int m = (int)v;
    if (m < 0 || m > data.n) { err = "invalid size of a class"; return false; }
    set<int> items;                       // removes repeated items of a class
    for (int a = 0; a < m; a++) {
      if (!next(v)) return false;
      if (v < 1 || v > data.n) { err = "item out of range in a class"; return false; }
      items.insert((int)v);
    }
    data.cls[t].assign(items.begin(), items.end());
  }
  if (pos != tok.size()) cout << "Warning: extra data at the end of the input file" << endl;
  data.Prepare();
  return true;
}

// Output format (lines starting with '#' are comments):
//   value of the best solution (number of bins)
//   best lower bound
//   number of bins
//   one line per bin with the items it contains
static bool WriteSolution(const string &filename, const BinPackingData &data) {
  ofstream out(filename.c_str());
  if (!out) return false;
  out << "# valor da solucao (numero de recipientes)\n" << data.best_value << "\n"
      << "# limitante inferior\n" << data.lower_bound << "\n"
      << "# numero de recipientes\n" << data.best_solution.size() << "\n"
      << "# itens de cada recipiente\n";
  for (size_t b = 0; b < data.best_solution.size(); b++) {
    for (size_t a = 0; a < data.best_solution[b].size(); a++)
      out << (a ? " " : "") << data.best_solution[b][a];
    out << "\n";
  }
  return true;
}

int main(int argc, char *argv[]) {
  string infile, outfile;
  double T = -1.0;
  unsigned seed = 1;
  for (int i = 1; i < argc; i++) {
    string arg = argv[i];
    if (arg == "-f" && i + 1 < argc) infile = argv[++i];
    else if (arg == "-o" && i + 1 < argc) outfile = argv[++i];
    else if (arg == "-t" && i + 1 < argc) T = atof(argv[++i]);
    else if (arg == "-r" && i + 1 < argc) seed = (unsigned)atol(argv[++i]);
    else { cout << "Unknown or incomplete argument: " << arg << endl; Usage(argv[0]); return 1; }
  }
  if (infile.empty() || outfile.empty() || T < 0) { Usage(argv[0]); return 1; }

  BinPackingData data;
  string err;
  if (!ReadInstance(infile, data, err)) {
    cout << "Error reading " << infile << ": " << err << endl;
    return 1;
  }
  data.seed = seed;
  cout << "Instance: n = " << data.n << ", C = " << data.C << ", k = " << data.k
       << " conflict classes; lower bound " << data.lower_bound << endl;

  SolveBinPacking(data, T);

  // The solution returned by the solver is checked here, independently.
  if (data.best_solution.empty()) {
    cout << "Error: the solver did not return any solution" << endl;
    return 1;
  }
  if (!data.CheckSolution(data.best_solution, err)) {
    cout << "Error: the solution is NOT feasible: " << err << endl;
    return 1;
  }
  if (data.best_value != (int)data.best_solution.size()) {
    cout << "Error: best_value does not match the number of bins" << endl;
    return 1;
  }
  cout << "Feasible solution with " << data.best_value << " bins (lower bound "
       << data.lower_bound << ")" << endl;
  if (!WriteSolution(outfile, data)) {
    cout << "Error: cannot write " << outfile << endl;
    return 1;
  }
  return 0;
}
