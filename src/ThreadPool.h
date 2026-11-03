#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <condition_variable>
#include <cstddef>
#include <exception>
#include <filesystem>
#include <mutex>
#include <queue>
#include <vector>
#include <thread>

#include "DataLoader.h"
#include "TextProcessor.h"
#include "InvertedIndex.h"

struct Task
{
    DocumentId documentId;
    std::filesystem::path path;
};

class ThreadPool
{
public:
    ThreadPool(
        int threadCount,
        const DataLoader& dataLoader,
        const TextProcessor& textProcessor,
        InvertedIndex& invertedIndex
    );

    ~ThreadPool();

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    void addTask(Task task);
    void wait();

private:
    void worker();
    void processDocument(const Task& task);
    void stop();

    const DataLoader& dataLoader_;
    const TextProcessor& textProcessor_;
    InvertedIndex& invertedIndex_;

    std::vector<std::thread> workers_;
    std::queue<Task> tasks_;

    std::mutex queueMutex_;
    std::mutex indexMutex_;

    std::condition_variable taskAvailable_;
    std::condition_variable allTasksFinished_;

    std::size_t activeTasks_ = 0;
    bool stopped_ = false;

    std::exception_ptr firstException_;
};

#endif //THREADPOOL_H
