#include "ThreadPool.h"

#include <stdexcept>
#include <utility>

ThreadPool::ThreadPool(
    int threadCount,
    const DataLoader& dataLoader,
    const TextProcessor& textProcessor,
    InvertedIndex& invertedIndex
)
    : dataLoader_(dataLoader),
      textProcessor_(textProcessor),
      invertedIndex_(invertedIndex)
{
    if (threadCount <= 0)
    {
        throw std::invalid_argument(
            "Thread count must be greater than zero."
        );
    }

    workers_.reserve(static_cast<std::size_t>(threadCount));

    try
    {
        for (int i = 0; i < threadCount; ++i)
        {
            workers_.emplace_back(&ThreadPool::worker, this);
        }
    }
    catch (...)
    {
        // Завершуємо потоки, які вже вдалося створити.
        stop();

        for (std::thread& workerThread : workers_)
        {
            workerThread.join();
        }

        throw;
    }
}

ThreadPool::~ThreadPool()
{
    stop();

    for (std::thread& workerThread : workers_)
    {
        workerThread.join();
    }
}

void ThreadPool::addTask(Task task)
{
    {
        std::lock_guard<std::mutex> lock(queueMutex_);

        if (stopped_)
        {
            throw std::runtime_error(
                "Cannot add a task to a stopped thread pool."
            );
        }

        tasks_.push(std::move(task));
    }

    taskAvailable_.notify_one();
}

void ThreadPool::worker()
{
    while (true)
    {
        Task task;

        {
            std::unique_lock<std::mutex> lock(queueMutex_);

            taskAvailable_.wait(lock, [this]()
            {
                return stopped_ || !tasks_.empty();
            });

            // Після зупинки допрацьовуємо всі прийняті завдання.
            if (stopped_ && tasks_.empty())
            {
                return;
            }

            task = std::move(tasks_.front());
            tasks_.pop();

            ++activeTasks_;
        }

        // Черга вже розблокована, інші потоки можуть брати завдання.
        std::exception_ptr taskException;

        try
        {
            processDocument(task);
        }
        catch (...)
        {
            taskException = std::current_exception();
        }

        {
            std::lock_guard<std::mutex> lock(queueMutex_);

            if (taskException && !firstException_)
            {
                firstException_ = taskException;
            }

            --activeTasks_;

            if (tasks_.empty() && activeTasks_ == 0)
            {
                allTasksFinished_.notify_all();
            }
        }
    }
}

void ThreadPool::processDocument(const Task& task)
{
    std::optional<Document> document = dataLoader_.loadDocument(task.documentId, task.path);

    if (!document)
    {
        return;
    }

    TermPositions termPositions = textProcessor_.collectTermPositions(document->text);

    // Текст більше не потрібен: звільняємо його до очікування м’ютекса.
    document.reset();

    {
        std::lock_guard<std::mutex> lock(indexMutex_);

        invertedIndex_.addDocument(task.documentId, std::move(termPositions));
    }
}

void ThreadPool::wait()
{
    std::exception_ptr exception;

    {
        std::unique_lock<std::mutex> lock(queueMutex_);

        allTasksFinished_.wait(lock, [this]()
        {
            return tasks_.empty() && activeTasks_ == 0;
        });

        exception = firstException_;
        firstException_ = nullptr;
    }

    // Передаємо помилку в потік, який викликав wait().
    if (exception)
    {
        std::rethrow_exception(exception);
    }
}

void ThreadPool::stop()
{
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        stopped_ = true;
    }

    taskAvailable_.notify_all();
}
