/*
The Heady library is distributed under the MIT License (MIT)
https://opensource.org/licenses/MIT
See LICENSE.TXT or Heady.h for license details.
Copyright (c) 2018 James Boer
*/

#include "Heady.h"

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
    namespace Internal
    {
        // Forward declaration
        void FindAndProcessLocalIncludes(const std::list<std::filesystem::directory_entry> & dirEntries, const std::filesystem::directory_entry & dirEntry, std::set<std::string> & processed, std::string & outputText);

        std::vector<std::string> Tokenize(const std::string & source)
        {
            if (source.empty())
                return {};
            std::regex r(R"(\s+)");
            std::sregex_token_iterator
                first{ source.begin(), source.end(), r, -1 },
                last;
            return { first, last };
        }

        bool EndsWith(std::string_view str, std::string_view suffix)
        {
            return str.size() >= suffix.size() && str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
        }
      
        void FindAndProcessLocalIncludes(const std::list<std::filesystem::directory_entry> & dirEntries, const std::string & include, std::set<std::string> & processed, std::string & outputText)
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

        void FindAndProcessLocalIncludes(const std::list<std::filesystem::directory_entry> & dirEntries, const std::filesystem::directory_entry & dirEntry, std::set<std::string> & processed, std::string & outputText)
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

	std::string GetVersionString()
	{
		std::array<char, 32> buffer;
		snprintf(buffer.data(), buffer.size(), "%i.%i.%i", MajorVersion, MinorVersion, PatchNumber);
		return buffer.data();
	}

	void GenerateHeader(std::string_view sourceFolder, std::string_view output, std::string_view excluded, bool recursive)
    {
        // Add initial file entries from designated source folder
		std::list<std::filesystem::directory_entry> dirEntries;
		if (recursive)
		{
			for (const auto & f : std::filesystem::recursive_directory_iterator(sourceFolder))
				dirEntries.emplace_back(f);
		}
		else
		{
			for (const auto & f : std::filesystem::directory_iterator(sourceFolder))
				dirEntries.emplace_back(f);
		}
        
        // Create list of excluded filenames
        auto excludedFilenames = Internal::Tokenize(std::string(excluded));

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

        // Recursively combine all source and headers into a single output string
        std::string outputText;
        std::set<std::string> processed;
        for (const auto & entry : dirEntries)
            Internal::FindAndProcessLocalIncludes(dirEntries, entry, processed, outputText);

        // Check to see if output folder exists.  If not, create it
        auto outFolder =  std::filesystem::path(output);
        outFolder.remove_filename();
        if (!std::filesystem::exists(outFolder))
        {
            std::filesystem::create_directory(outFolder);
        }
        else
        {
		    // Remove existing file
		    if (std::filesystem::exists(output))
			    std::filesystem::remove(output);
        }

		// Write all processed file data to new header file
		std::ofstream outFile;
		outFile.open(output, std::ios::out);
        outFile << outputText;
    }
}