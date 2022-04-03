
// Amalgamation-specific define
#ifndef HEADY_HEADER_ONLY
#define HEADY_HEADER_ONLY
#endif


// begin --- Heady.cpp --- 

/*
The Heady library is distributed under the MIT License (MIT)
https://opensource.org/licenses/MIT
See LICENSE.TXT or Heady.h for license details.
Copyright (c) 2018 James Boer
*/

// begin --- Heady.h --- 

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

#pragma once

#include <string>

#define inline_t

namespace Heady
{
	/// Major version number
	const uint32_t MajorVersion = 0;

	/// Minor version number
	const uint32_t MinorVersion = 2;

	/// Patch number
	const uint32_t PatchNumber = 3;

	/// Get the version number in string form
	std::string GetVersionString();

	struct Params
	{
		std::string sourceFolder;
		std::string output;
		std::string excluded;
		std::string inlined;
		std::string define;
		bool recursiveScan;
	};

	/// Generate combined header from source
	void GenerateHeader(const Params& params);

}


// end --- Heady.h --- 



#include <array>
#include <vector>
#include <list>
#include <map>
#include <set>
#include <filesystem>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <iterator>
#include <regex>

namespace Heady
{
	namespace Detail
	{
		// Forward declaration
		void FindAndProcessLocalIncludes(const std::list<std::filesystem::directory_entry> & dirEntries, const std::filesystem::directory_entry & dirEntry, std::set<std::string> & processed, std::string & outputText);

		inline std::vector<std::string> Tokenize(const std::string & source)
		{
			if (source.empty())
				return {};
			std::regex r(R"(\s+)");
			std::sregex_token_iterator
				first{ source.begin(), source.end(), r, -1 },
				last;
			return { first, last };
		}

		inline bool EndsWith(std::string_view str, std::string_view suffix)
		{
			return str.size() >= suffix.size() && str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
		}

		inline void FindAndReplaceAll(std::string& str, std::string_view search, std::string_view replace)
		{
			size_t pos = str.find(search);
			while (pos != std::string::npos)
			{
				str.replace(pos, search.size(), replace);
				pos = str.find(search, pos + search.size());
			}
		}
	  
		inline void FindAndProcessLocalIncludes(const std::list<std::filesystem::directory_entry> & dirEntries, const std::string & include, std::set<std::string> & processed, std::string & outputText)
		{
			// Check to see if we've already processed this file
			if (processed.find(include) != processed.end())
				return;

			// Find the directory entry that matches this include filename, and if found, process it
			auto itr = std::find_if(dirEntries.begin(), dirEntries.end(), [include](const auto & entry)
			{
				return EndsWith(entry.path().string(), include);
			});
			if (itr != dirEntries.end())
			{
				FindAndProcessLocalIncludes(dirEntries, *itr, processed, outputText);
			}
		}

		inline void FindAndProcessLocalIncludes(const std::list<std::filesystem::directory_entry> & dirEntries, const std::filesystem::directory_entry & dirEntry, std::set<std::string> & processed, std::string & outputText)
		{
			// Check to see if we've already processed this file
			auto fn = dirEntry.path().filename().string();
			if (processed.find(fn) != processed.end())
				return;

			// Now mark this file as processed, so we don't add it twice to the combined header
			processed.emplace(fn);

			// Open and read file from dirEntry
			std::ifstream file(dirEntry.path());
			std::stringstream buffer;
			buffer << file.rdbuf();
			std::string fileData = buffer.str();

			// Mark file beginning
			outputText += "\n\n// begin --- ";
			outputText += dirEntry.path().filename().string();
			outputText += " --- ";
			outputText += "\n\n";

			// Find local includes
			std::regex r(R"regex(\s*#\s*include\s*(["])([^"]+)(["]))regex");
			std::smatch m;
			std::string s = fileData;
			while (std::regex_search(s, m, r))
			{
				// Insert text found up to the include match
				if (m.prefix().length())
					outputText += m.prefix().str();

				// Insert the include text into the output stream
				FindAndProcessLocalIncludes(dirEntries, m[2].str(), processed, outputText);

				// Continue processing the rest of the file text
				s = m.suffix().str();
			}

			// Copy remaining file text to output
			outputText += s;

			// Mark file end
			outputText += "\n\n// end --- ";
			outputText += dirEntry.path().filename().string();
			outputText += " --- ";
			outputText += "\n\n";
		}
	}

	inline std::string GetVersionString()
	{
		std::array<char, 32> buffer;
		snprintf(buffer.data(), buffer.size(), "%i.%i.%i", MajorVersion, MinorVersion, PatchNumber);
		return buffer.data();
	}

	inline void GenerateHeader(const Params& params)
	{
		if (params.output.empty())
			throw std::invalid_argument("Requires a valid output argument");

		// Add initial file entries from designated source folder
		std::list<std::filesystem::directory_entry> dirEntries;
		if (params.recursiveScan)
		{
			for (const auto & f : std::filesystem::recursive_directory_iterator(params.sourceFolder))
				dirEntries.emplace_back(f);
		}
		else
		{
			for (const auto & f : std::filesystem::directory_iterator(params.sourceFolder))
				dirEntries.emplace_back(f);
		}
		
		// Create list of excluded filenames
		auto excludedFilenames = Detail::Tokenize(params.excluded);

		// Remove excluded files from fileEntries
		dirEntries.remove_if([&excludedFilenames](const auto & entry)
		{
			for (auto fn : excludedFilenames)
			{
				if ((entry.path().filename()) == fn)
					return true;
			}
			return false;
		});

		// No need to do anything if we don't have any files to process
		if (dirEntries.empty())
			return;

		// Make sure .cpp files are processed first
		dirEntries.sort([](const auto & left, const auto & right)
		{
			// We're taking advantage of the fact that cpp < h or hpp or inc.  If we need to add other
			// extensions, we'll have to revisit this.
			return left.path().extension() < right.path().extension();
		});

		// Amalgamation-specific define for header
		std::string outputText;
		if (!params.define.empty())
		{
			outputText += "\n// Amalgamation-specific define";
			outputText += "\n#ifndef ";
			outputText += params.define;
			outputText += "\n#define ";
			outputText += params.define;
			outputText += "\n#endif\n";
		}

		// Recursively combine all source and headers into a single output string
		std::set<std::string> processed;
		for (const auto & entry : dirEntries)
			Detail::FindAndProcessLocalIncludes(dirEntries, entry, processed, outputText);

		// Replace all instances of a specified macro with 'inline'
		std::string inlineValue = params.inlined;
		if (inlineValue.empty())
			inlineValue = "inline ";
		if (inlineValue[inlineValue.size() - 1] != ' ')
			inlineValue += " ";
		Detail::FindAndReplaceAll(outputText, inlineValue, "inline ");

		// Check to see if output folder exists.  If not, create it
		auto outFolder =  std::filesystem::path(params.output);
		outFolder.remove_filename();
		if (!std::filesystem::exists(outFolder))
		{
			std::filesystem::create_directory(outFolder);
		}
		else
		{
			// Remove existing file
			if (std::filesystem::exists(params.output))
				std::filesystem::remove(params.output);
		}

		// Write all processed file data to new header file
		std::ofstream outFile;
		outFile.open(params.output, std::ios::out);
		outFile << outputText;
	}
	
}

// end --- Heady.cpp --- 

