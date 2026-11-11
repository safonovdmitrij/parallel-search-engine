#include "IndexService.h"

#include <chrono>
#include <stdexcept>
#include <fstream>
#include <utility>

IndexService::IndexService(
    const DataLoader& dataLoader,
    const TextProcessor& textProcessor
)
    : dataLoader_(dataLoader),
      textProcessor_(textProcessor),
      current_(std::make_unique<IndexState>())
{
}

std::optional<IndexStatistics> IndexService::rebuild(int threadCount)
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

    auto results = searchService.search(query);

    for (auto& result : results)
    {
        std::ifstream file(current_->documentPaths.at(result.documentId),std::ios::binary);

        if (!file)
        {
            continue;
        }

        // Короткий фрагмент із початку документа.
        std::string text(240, '\0');

        file.read(text.data(),static_cast<std::streamsize>(text.size()));
        text.resize(static_cast<std::size_t>(file.gcount()));

        // Не залишаємо обрізаний UTF-8 символ наприкінці.
        if (text.size() == 240)
        {
            while (!text.empty() && (static_cast<unsigned char>(text.back()) & 0xC0) == 0x80)
            {
                text.pop_back();
            }

            if (!text.empty() && static_cast<unsigned char>(text.back()) >= 0xC0)
            {
                text.pop_back();
            }
        }

        for (char& character : text)
        {
            if (character == '\n' || character == '\r' || character == '\t')
            {
                character = ' ';
            }
        }

        result.snippet = std::move(text);
    }

    return results;
}

IndexStatistics IndexService::getStatistics() const
{
    std::shared_lock<std::shared_mutex> lock(stateMutex_);

    return current_->statistics;
}