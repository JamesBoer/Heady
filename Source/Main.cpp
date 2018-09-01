/*
The MIT License (MIT)

Copyright (c) 2018 James Boer

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include <iostream>
#include <thread>
#include <cstring>
#include "clara.hpp"
#include "Heady.h"

using namespace Heady;
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
        Help(showHelp);

	auto result = parser.parse(Args(argc, argv));
	if (!result)
	{
		std::cerr << "Error in command line: " << result.errorMessage() << std::endl;
		return 1;
	}
	else if (showHelp)
	{
		parser.writeToStream(std::cout);
		std::cout << 
			"\nExample usage:" << 
			"\nHeady --source \"\\Source\" --exluded \"Main.cpp clara.hpp\" --output \"\\Include\\Heady.hpp\"";
		return 0;
	}

    // Generate a combined header file from all C++ source files
	try
	{
		GenerateHeader(source, output, excluded, recursive);
	}
	catch (const std::exception & e)
	{
		std::cerr << "Error processing source files.  " << e.what() << std::endl;
		return 1;
	}

    return 0;
}

