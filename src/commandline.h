#ifndef COMMANDLINE_H
#define COMMANDLINE_H

#include <optional>
#include <string>

struct CommandLineOptions
{
	std::string input_file;
	std::string output_file;
	double time_limit = -1.0;
	unsigned seed = 1;
};

// Returns validated options, or nullopt after printing an error or usage message.
std::optional<CommandLineOptions> ParseCommandLine(int argc, const char *const argv[]);

#endif
