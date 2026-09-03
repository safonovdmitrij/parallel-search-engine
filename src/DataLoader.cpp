#include "DataLoader.h"

#include <iostream>
#include <algorithm>

DataLoader::DataLoader(const std::filesystem::path& dataDirectory)
    : dataDirectory_(dataDirectory)
{
}

std::vector<std::filesystem::path> DataLoader::getDocumentPaths() const
{
    std::vector<std::filesystem::path> documentPaths;

    for(const std::filesystem::directory_entry& entry :
        std::filesystem::recursive_directory_iterator(dataDirectory_))
    {
        if(entry.is_regular_file() && entry.path().extension() == ".txt")
        {
            documentPaths.push_back(entry.path());
        }
    }

    std::sort(documentPaths.begin(), documentPaths.end());

    return documentPaths;
}
