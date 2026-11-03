#ifndef SEARCHSERVICE_H
#define SEARCHSERVICE_H

#include "InvertedIndex.h"
#include "TextProcessor.h"

#include <string>
#include <vector>

struct SearchResult
{
    DocumentId documentId;
    std::vector<std::string> matchedTerms;
};

class SearchService
{
public:
    SearchService(
        const InvertedIndex& invertedIndex,
        const TextProcessor& textProcessor
    );

    std::vector<SearchResult> search(
        const std::string& query
    ) const;

private:
    const InvertedIndex& invertedIndex_;
    const TextProcessor& textProcessor_;
};

#endif //SEARCHSERVICE_H

