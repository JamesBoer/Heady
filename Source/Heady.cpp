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

        std::list<std::string> FindAndRemoveLocalIncludes(std::string & source)
        {
            std::list<std::string> includes;
            std::regex r(R"regex(\s*#\s*include\s*(["])([^"]+)(["]))regex");
            std::smatch m;
            std::string s = source;
            source = "";
            while (std::regex_search(s, m, r))
            {
                includes.push_back(m[2].str());
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

		void AddDependencies(const FileEntryDependencies & fileEntries, const std::list<std::filesystem::directory_entry> & dependencies, std::list<std::filesystem::directory_entry> & sortedFileEntries)
		{
			// Look at all dependencies
			for (const auto & dep : dependencies)
			{
				// Check to see if we're already in the sorted list.  If not, check for sub-dependencies, then add it
				if (std::find(sortedFileEntries.begin(), sortedFileEntries.end(), dep) == sortedFileEntries.end())
				{
					// Check to see if these dependencies have dependencies of their own.
					auto itr = std::find_if(fileEntries.begin(), fileEntries.end(), [&dep](const auto & pair) { return pair.first == dep; });
					if (itr != fileEntries.end())
						AddDependencies(fileEntries, itr->second, sortedFileEntries);

					// Check one more time to make sure we don't have circular dependencies
					if (std::find(sortedFileEntries.begin(), sortedFileEntries.end(), dep) == sortedFileEntries.end())
						sortedFileEntries.push_back(dep);
				}
			}
		}

        std::list<std::filesystem::directory_entry> SortFileEntries(const FileEntryDependencies & fileEntries)
        {
			// This function creates a file entry list sorted by dependency requirements

			// Sorted file entries
            std::list<std::filesystem::directory_entry> sortedFileEntries;

			// Iterate through all files in the original list
			for (const auto & entryPair : fileEntries)
			{
				// Recursively add dependencies from each file entry
				AddDependencies(fileEntries, entryPair.second, sortedFileEntries);

				// If we aren't already in the list, then add the file
				if (std::find(sortedFileEntries.begin(), sortedFileEntries.end(), entryPair.first) == sortedFileEntries.end())
					sortedFileEntries.push_back(entryPair.first);
			}

            return sortedFileEntries;
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
		Internal::FileEntryDependencies fileEntries;
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
        auto excludedFilenames = Internal::Tokenize(std::string(excluded));

        // Remove excluded files from fileEntries
        fileEntries.remove_if([&excludedFilenames](const auto & pair)
        {
            for (auto fn : excludedFilenames)
            {
                if ((pair.first.path().filename()) == fn)
                    return true;
            }
            return false;
        });

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
            auto includes = Internal::FindAndRemoveLocalIncludes(fileData);

			// Store processed data in a map
			fileDataMap[fp.first] = fileData;
            
            // Find all dependencies from within the original fileEntries list, given the set of strings returned
            for (const auto & inc : includes)
            {
                for (const auto & fp2 : fileEntries)
                {
                    if (Internal::EndsWith(fp2.first.path().string(), inc))
                    {
                        fp.second.push_back(fp2.first);
                        break;
                    }
                }
            }
        }

		// Make sure .cpp files are processed first
		fileEntries.sort([](const auto & left, const auto & right)
		{
			// We're taking advantage of the fact that cpp < h or hpp or inc.  If we need to add other
			// extensions, we'll have to revisit this.
			return left.first.path().extension() < right.first.path().extension();
		});

		// Sort all file entries by dependency
        auto sortedFileEntries = Internal::SortFileEntries(fileEntries);

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