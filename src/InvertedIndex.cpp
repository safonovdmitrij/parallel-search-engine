#include "InvertedIndex.h"

#include <utility>

void InvertedIndex::addDocument(DocumentId documentId, const std::vector<std::string>& tokens)
{
    std::unordered_map<std::string, std::vector<std::size_t>> termPositions;

    for (std::size_t position = 0; position < tokens.size(); position++)
    {
        const std::string& term = tokens[position];
        termPositions[term].push_back(position);
    }

    for (auto& [term, positions] : termPositions)
    {
        index_[term].push_back(Posting{documentId, std::move(positions)});
    }

    ++documentsCount_;
}


std::vector<Posting> InvertedIndex::getPostings(const std::string& term) const
{
    const auto iterator = index_.find(term);

    if (iterator == index_.end())
    {
        return {};
    }

    return iterator->second;
}


std::size_t InvertedIndex::getDocumentsCount() const
{
    return documentsCount_;
}