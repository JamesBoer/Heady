/*
The Heady library is distributed under the MIT License (MIT)
https://opensource.org/licenses/MIT
See LICENSE.TXT or Heady.h for license details.
Copyright (c) 2018 James Boer
*/

#include "Heady.h"

#include <vector>
#include <list>
#include <set>
#include <map>
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
		using FileEntryDependencies = std::list<std::pair<std::filesystem::directory_entry, std::list<std::filesystem::directory_entry>>>;

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

        std::set<std::string> FindAndRemoveLocalIncludes(std::string & source)
        {
            std::set<std::string> includes;
            std::regex r(R"regex(\s*#\s*include\s*(["])([^"]+)(["]))regex");
            std::smatch m;
            std::string s = source;
            source = "";
            while (std::regex_search(s, m, r))
            {
                includes.emplace(m[2].str());
                if (m.prefix().length())
                    source += m.prefix().str();
                s = m.suffix().str();
            }
            source += s;
            return includes;
        }

        bool EndsWith(std::string_view str, std::string_view suffix)
        {
            return str.size() >= suffix.size() && str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
        }

        std::list<std::filesystem::directory_entry> SortFileEntries(const FileEntryDependencies & fileEntries)
        {
			// This function creates a file entry list sorted by dependency requirements

			// Sorted file entries
            std::list<std::filesystem::directory_entry> sortedFileEntries;

			// Continue until we've added all file entries to the sorted list
			while (sortedFileEntries.size() < fileEntries.size())
			{
				// Iterate through all files in the original list
				for (const auto & entryPair : fileEntries)
				{
					// Check all dependencies, and if we're missing any dependencies already in the list
					bool missingDependency = false;
					for (const auto & dep : entryPair.second)
					{
						// If the depenency isn't in the current list, we mark it as a missing dependency
						if (std::find(sortedFileEntries.begin(), sortedFileEntries.end(), dep) == sortedFileEntries.end())
						{
							missingDependency = true;
							break;
						}
					}

					// If we don't have a missing dependency AND we aren't already in the list, then add this file
					if (!missingDependency && std::find(sortedFileEntries.begin(), sortedFileEntries.end(), entryPair.first) == sortedFileEntries.end())
					{
						sortedFileEntries.push_back(entryPair.first);
					}
				}
			}

            return sortedFileEntries;
        }
    }

	void GenerateHeader(std::string_view sourceFolder, std::string_view output, std::string_view excluded, bool recursive)
    {
		using namespace Internal;

        // Add initial file entries from designated source folder
		FileEntryDependencies fileEntries;
		if (recursive)
		{
			for (const auto & f : std::filesystem::recursive_directory_iterator(sourceFolder))
				fileEntries.push_back(std::make_pair(f, std::list<std::filesystem::directory_entry>()));
		}
		else
		{
			for (const auto & f : std::filesystem::directory_iterator(sourceFolder))
				fileEntries.push_back(std::make_pair(f, std::list<std::filesystem::directory_entry>()));
		}
        
        // Create list of excluded filenames
        auto excludedFilenames = Tokenize(std::string(excluded));        

        // Remove excluded files from fileEntries
        if (!excludedFilenames.empty())
        {
            fileEntries.remove_if([&excludedFilenames](const auto & pair)
            {
                if (!excludedFilenames.empty())
                {
                    for (auto fn : excludedFilenames)
                    {
                        if ((pair.first.path().filename()) == fn)
                            return true;
                    }
                    return false;
                }
                return false;
            });
        }

        // No need to do anything if we don't have any files to process
		if (fileEntries.empty())
			return;

		// Store file data in a map for later processing
		std::map<std::filesystem::directory_entry, std::string> fileDataMap;

        // Read and analyze each file
        std::string combinedHeader;
        for (auto & fp : fileEntries)
        {
			// Open and read source file
			std::ifstream file(fp.first.path());
			std::stringstream buffer;
			buffer << file.rdbuf();
			std::string fileData = buffer.str();

            // Strip local include lines out, and return them in a set
            auto includes = FindAndRemoveLocalIncludes(fileData);

			// Store processed data in a map
			fileDataMap[fp.first] = fileData;
            
            // Find all dependencies from within the original fileEntries list, given the set of strings returned
            for (const auto & inc : includes)
            {
                for (const auto & fp2 : fileEntries)
                {
                    if (EndsWith(fp2.first.path().string(), inc))
                    {
                        fp.second.push_back(fp2.first);
                        break;
                    }
                }
            }
        }

		// Sort all file entries by dependency
        auto sortedFileEntries = SortFileEntries(fileEntries);

		// Remove existing file
		if (std::filesystem::exists(output))
			std::filesystem::remove(output);

		// Write all processed file data to new header file
		std::ofstream outFile;
		outFile.open(output, std::ios::out);
		for (auto & fileEntry : sortedFileEntries)
		{
			auto fileName = fileEntry.path().filename();
			outFile << "\n" << "/* begin --- " << fileName << " --- */ \n";
			outFile << fileDataMap[fileEntry];
			outFile << "\n\n" << "/* end --- " << fileName << " --- */\n";
		}
		outFile.close();
    }
}