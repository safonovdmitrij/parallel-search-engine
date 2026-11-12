#ifndef INVERTEDINDEX_H
#define INVERTEDINDEX_H

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

#include "Document.h"
#include "TermPositions.h"

struct Posting
{
    DocumentId documentId;
    std::vector<TokenPosition> positions;
};

class InvertedIndex
{
public:
    void addDocument(DocumentId documentId, TermPositions termPositions);

    std::size_t getDocumentsCount() const;
    const std::vector<Posting>& getPostings(const std::string& term) const;

private:
    std::unordered_map<std::string, std::vector<Posting>> index_;
    std::size_t documentsCount_ = 0;
};

#endif