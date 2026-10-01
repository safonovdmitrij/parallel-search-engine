#include "Indexer.h"

Indexer::Indexer(
    const DataLoader& dataLoader,
    const TextProcessor& textProcessor,
    InvertedIndex& invertedIndex
) : dataLoader_(dataLoader), textProcessor_(textProcessor), invertedIndex_(invertedIndex)
{
}


std::vector<std::filesystem::path> Indexer::build()
{
    const std::vector<std::filesystem::path> documentPaths = dataLoader_.getDocumentPaths();

    for (DocumentId documentId = 0; documentId < documentPaths.size(); ++documentId)
    {
        const std::optional<Document> document = dataLoader_.loadDocument(documentId, documentPaths[documentId]);

        if (!document)
        {
            continue;
        }

        const std::vector<std::string> tokens = textProcessor_.tokenize(document->text);
        invertedIndex_.addDocument(document->id, tokens);
    }

    return documentPaths;
}
