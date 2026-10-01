#include "SearchService.h"

#include <stdexcept>

SearchService::SearchService(
    const InvertedIndex& invertedIndex,
    const TextProcessor& textProcessor
) : invertedIndex_(invertedIndex), textProcessor_(textProcessor)
{
}

std::vector<Posting> SearchService::search(const std::string& query) const
{
    const std::vector<std::string> tokens = textProcessor_.tokenize(query);

    if (tokens.empty())
    {
        return {};
    }

    if (tokens.size() > 1)
    {
        throw std::invalid_argument("Only single-term queries are supported.");
    }

    return invertedIndex_.getPostings(tokens.front());
}