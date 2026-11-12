#include "InvertedIndex.h"

#include <utility>

void InvertedIndex::addDocument(DocumentId documentId, TermPositions termPositions)
{
    for (auto& [term, positions] : termPositions)
    {
        index_[term].push_back(Posting{documentId, std::move(positions)});
    }

    ++documentsCount_;
}

const std::vector<Posting>& InvertedIndex::getPostings(const std::string& term) const
{
    const auto iterator = index_.find(term);

    if (iterator == index_.end())
    {
        static const std::vector<Posting> empty;
        return empty;
    }

    return iterator->second;
}

std::size_t InvertedIndex::getDocumentsCount() const
{
    return documentsCount_;
}