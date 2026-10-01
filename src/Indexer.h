#ifndef INDEXER_H
#define INDEXER_H

#include "DataLoader.h"
#include "InvertedIndex.h"
#include "TextProcessor.h"

class Indexer
{
public:
    Indexer(const DataLoader& dataLoader, const TextProcessor& textProcessor, InvertedIndex& invertedIndex);

    std::vector<std::filesystem::path> build();

private:
    const DataLoader& dataLoader_;
    const TextProcessor& textProcessor_;
    InvertedIndex& invertedIndex_;
};

#endif //INDEXER_H
