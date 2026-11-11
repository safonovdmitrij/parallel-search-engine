#include "SearchService.h"

#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <utility>

SearchService::SearchService(
    const InvertedIndex& invertedIndex,
    const TextProcessor& textProcessor
)
    : invertedIndex_(invertedIndex),
      textProcessor_(textProcessor)
{
}

std::vector<SearchResult> SearchService::search(const std::string& query) const
{
    const std::vector<std::string> tokens = textProcessor_.tokenize(query);

    std::unordered_set<std::string> processedTerms;

    std::unordered_map<DocumentId, SearchResult> documentResults;

    for (const std::string& term : tokens)
    {
        // Кожен термін запиту обробляємо лише один раз.
        if (!processedTerms.insert(term).second)
        {
            continue;
        }

        const std::vector<Posting> postings = invertedIndex_.getPostings(term);

        for (const Posting& posting : postings)
        {
            auto [iterator, inserted] = documentResults.try_emplace(
                posting.documentId,
                SearchResult{posting.documentId, {}}
            );

            iterator->second.matchedTerms.push_back(term);
        }
    }

    std::vector<SearchResult> results;
    results.reserve(documentResults.size());

    for (auto& entry : documentResults)
    {
        results.push_back(std::move(entry.second));
    }

    std::sort(
        results.begin(),
        results.end(),
        [](const SearchResult& left, const SearchResult& right)
        {
            if (left.matchedTerms.size() != right.matchedTerms.size())
            {
                return left.matchedTerms.size() > right.matchedTerms.size();
            }

            return left.documentId < right.documentId;
        }
    );

    return results;
}