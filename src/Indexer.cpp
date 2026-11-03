#include "Indexer.h"
#include "ThreadPool.h"

Indexer::Indexer(
    const DataLoader& dataLoader,
    const TextProcessor& textProcessor,
    InvertedIndex& invertedIndex
) : dataLoader_(dataLoader), textProcessor_(textProcessor), invertedIndex_(invertedIndex)
{
}


std::vector<std::filesystem::path> Indexer::build(int threadCount)
{
    const std::vector<std::filesystem::path> documentPaths =
        dataLoader_.getDocumentPaths();

    ThreadPool pool(
        threadCount,
        dataLoader_,
        textProcessor_,
        invertedIndex_
    );

    for (DocumentId documentId = 0;
         documentId < documentPaths.size();
         ++documentId)
    {
        pool.addTask({
            documentId,
            documentPaths[documentId]
        });
    }

    pool.wait();

    return documentPaths;
}
