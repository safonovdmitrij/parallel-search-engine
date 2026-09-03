#ifndef DATALOADER_H
#define DATALOADER_H

#include <filesystem>
#include <vector>

class DataLoader
{
public:
    explicit DataLoader(const std::filesystem::path& dataDirectory);

    std::vector<std::filesystem::path> getDocumentPaths() const;
    

private:
    std::filesystem::path dataDirectory_;
};

#endif //DATALOADER_H
