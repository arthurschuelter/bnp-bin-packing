#include "commandline.h"
#include <cerrno>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Usage(const char *prog)
{
	std::cout << "Usage: " << prog << " -f <input file> -o <output file> -t <seconds> [-r <seed>]"
	          << std::endl
	          << "  -f  input file with the instance" << std::endl
	          << "  -o  output file where the best solution is written" << std::endl
	          << "  -t  maximum running time T, in seconds" << std::endl
	          << "  -r  seed of the random number generator (default 1)" << std::endl;
}
} // namespace

std::optional<CommandLineOptions> ParseCommandLine(int argc, const char *const argv[])
{
	CommandLineOptions options;
	for (int i = 1; i < argc; i++)
	{
		std::string arg = argv[i];
		if (arg == "-f" && i + 1 < argc)
			options.input_file = argv[++i];
		else if (arg == "-o" && i + 1 < argc)
			options.output_file = argv[++i];
		else if (arg == "-t" && i + 1 < argc)
		{
			const char *text = argv[++i];
			char *end = nullptr;
			errno = 0;
			options.time_limit = std::strtod(text, &end);
			// steady_clock duration and deadline arithmetic must be representable.
			double max_seconds =
			    std::chrono::duration<double>(std::chrono::steady_clock::time_point::max() -
				                              std::chrono::steady_clock::now())
			        .count();
			if (errno || end == text || *end || !std::isfinite(options.time_limit) ||
			    options.time_limit < 0 || options.time_limit >= max_seconds)
			{
				std::cerr << "Invalid time limit\n";
				return std::nullopt;
			}
		}
		else if (arg == "-r" && i + 1 < argc)
		{
			const char *text = argv[++i];
			char *end = nullptr;
			errno = 0;
			unsigned long value = std::strtoul(text, &end, 10);
			if (errno || end == text || *end || text[0] == '-' ||
			    value > std::numeric_limits<unsigned>::max())
			{
				std::cerr << "Invalid seed\n";
				return std::nullopt;
			}
			options.seed = static_cast<unsigned>(value);
		}
		else
		{
			std::cout << "Unknown or incomplete argument: " << arg << std::endl;
			Usage(argv[0]);
			return std::nullopt;
		}
	}
	if (options.input_file.empty() || options.output_file.empty() || options.time_limit < 0)
	{
		Usage(argv[0]);
		return std::nullopt;
	}
	return options;
}
