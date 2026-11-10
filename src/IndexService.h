#ifndef INDEXSERVICE_H
#define INDEXSERVICE_H

#include "Indexer.h"
#include "SearchService.h"

#include <memory>
#include <mutex>
#include <optional>
#include <shared_mutex>

struct IndexStatistics
{
    std::size_t documents = 0;
    double timeMs = 0;
    int threadsUsed = 0;
};

class IndexService
{
public:
    IndexService(
        const DataLoader& dataLoader,
        const TextProcessor& textProcessor
    );

    std::optional<IndexStatistics> rebuild(int threadCount);
    std::vector<SearchResult> search(const std::string& query) const;
    IndexStatistics getStatistics() const;

private:
    struct IndexState
    {
        InvertedIndex index;
        std::vector<std::filesystem::path> documentPaths;
        IndexStatistics statistics;
    };

    const DataLoader& dataLoader_;
    const TextProcessor& textProcessor_;

    std::unique_ptr<IndexState> current_;

    mutable std::shared_mutex stateMutex_;
    std::mutex rebuildMutex_;
 };

#endif //INDEXSERVICE_H
