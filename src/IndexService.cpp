#include "IndexService.h"

#include <chrono>
#include <stdexcept>

IndexService::IndexService(
    const DataLoader& dataLoader,
    const TextProcessor& textProcessor
)
    : dataLoader_(dataLoader),
      textProcessor_(textProcessor),
      current_(std::make_unique<IndexState>())
{
}

std::optional<IndexStatistics> IndexService::rebuild(
    int threadCount
)
{
    if (threadCount < 1 || threadCount > 128)
    {
        throw std::invalid_argument(
            "Thread count must be between 1 and 128."
        );
    }

    // Дозволяємо лише одну перебудову одночасно.
    std::unique_lock<std::mutex> buildLock(
        rebuildMutex_,
        std::try_to_lock
    );

    if (!buildLock.owns_lock())
    {
        return std::nullopt;
    }

    auto next = std::make_unique<IndexState>();

    const auto start = std::chrono::steady_clock::now();

    Indexer indexer(
        dataLoader_,
        textProcessor_,
        next->index
    );

    next->documentPaths = indexer.build(threadCount);

    const auto finish = std::chrono::steady_clock::now();

    next->statistics.documents = next->index.getDocumentsCount();

    next->statistics.timeMs = std::chrono::duration<double, std::milli>(finish - start).count();

    next->statistics.threadsUsed = threadCount;

    const IndexStatistics statistics = next->statistics;

    {
        // Очікуємо завершення активних пошуків.
        std::unique_lock<std::shared_mutex> stateLock(stateMutex_);

        // Міняємо лише вказівники, без копіювання індексу.
        current_.swap(next);
    }

    return statistics;
}

std::vector<SearchResult> IndexService::search(const std::string& query) const
{
    std::shared_lock<std::shared_mutex> stateLock(stateMutex_);

    SearchService searchService(
        current_->index,
        textProcessor_
    );

    return searchService.search(query);
}