#ifndef INVERTEDINDEX_H
#define INVERTEDINDEX_H

#include "Document.h"

#include <unordered_map>
#include <vector>
#include <string>
#include <cstddef>


struct Posting
{
    DocumentId documentId;
    std::vector<std::size_t> positions;
};


class InvertedIndex
{
public:
    void addDocument(DocumentId documentId, const std::vector<std::string>& tokens);
    std::vector<Posting> getPostings(const std::string& term) const;

private:
    std::unordered_map<std::string, std::vector<Posting>> index_;
};

#endif //INVERTEDINDEX_H
