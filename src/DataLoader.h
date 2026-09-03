#ifndef DATALOADER_H
#define DATALOADER_H

#include "Document.h"

#include <filesystem>
#include <vector>
#include <optional>

class DataLoader
{
public:
    explicit DataLoader(const std::filesystem::path& dataDirectory);

    std::vector<std::filesystem::path> getDocumentPaths() const;
    std::optional<Document> loadDocument(DocumentId id, const std::filesystem::path& documentPath) const;

private:
    std::filesystem::path dataDirectory_;
};

#endif //DATALOADER_H
