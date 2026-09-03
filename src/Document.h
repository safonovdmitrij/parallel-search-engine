#ifndef DOCUMENT_H
#define DOCUMENT_H

#include <filesystem>
#include <string>
#include <cstddef>

using DocumentId = std::size_t;

struct Document
{
    DocumentId id;
    std::filesystem::path path;
    std::string text;
};

#endif //DOCUMENT_H
