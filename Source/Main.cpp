/*
The Heady library is distributed under the MIT License (MIT)
https://opensource.org/licenses/MIT
See LICENSE.TXT or Heady.h for license details.
Copyright (c) 2018 James Boer
*/

#include <iostream>
#include <thread>
#include <cstring>
#include "clara.hpp"
#include "Heady.h"

using namespace clara;

int main(int argc, char ** argv)
{
	// Handle command-line options
	std::string source;
	std::string output;
    std::string excluded;
	bool recursive = false;
	bool showHelp = false;
	auto parser = 
		Opt(source, "source")["-s"]["--source"]("folder containing source files") |
        Opt(excluded, "excluded")["-e"]["--excluded"]("exclude specific files") |
		Opt(output, "output")["-o"]["--output"]("output filename for generated header file") |
		Opt(recursive, "recursive")["-r"]["--recursive"]["recursively scan source folder"] |
		Help(showHelp)
        ;

	auto result = parser.parse(Args(argc, argv));
	if (!result)
	{
		std::cerr << "Error in command line: " << result.errorMessage() << std::endl;
		return 1;
	}
	else if (showHelp)
	{
		std::cout << "Heady version " << Heady::GetVersionString() << " Copyright (c) James Boer\n\n";
		parser.writeToStream(std::cout);
		std::cout << 
			"\nExample usage:" << 
			"\nHeady --source \"\\Source\" --exluded \"Main.cpp clara.hpp\" --output \"\\Include\\Heady.hpp\"";
		return 0;
	}
	else if (source.empty() || output.empty())
	{
		std::cerr << "Error: Valid source (-s or --source) and output (-o or --output) are required.\n";
		return 1;
	}

    // Generate a combined header file from all C++ source files
	try
	{
		Heady::GenerateHeader(source, output, excluded, recursive);
	}
	catch (const std::exception & e)
	{
		std::cerr << "Error processing source files.  " << e.what() << std::endl;
		return 1;
	}

    return 0;
}

