#include "WebSocketController.h"

#include <algorithm>
#include <chrono>
#include <exception>
#include <optional>
#include <utility>

#include "TextProcessor.h"

namespace
{
    nlohmann::json makeErrorMessage(const std::string& message)
    {
        return {
            {"stage", "error"},
            {"data", {{"error", message}}}
        };
    }

    std::optional<std::string> parseSearchMessage(const std::string& message, std::string& query)
    {
        const nlohmann::json input = nlohmann::json::parse(message, nullptr, false);

        if (!input.is_object() || !input.contains("query") || !input["query"].is_string())
        {
            return "JSON field 'query' must be a string.";
        }

        query = input["query"].get<std::string>();

        if (query.size() > 1000 || query.find_first_not_of(" \t\r\n") == std::string::npos)
        {
            return "Query must contain text and not exceed 1000 bytes.";
        }

        return std::nullopt;
    }

    std::vector<std::string> getExpandedTerms(const std::string& query, const std::string& effectiveQuery)
    {
        // expandQuery() повертає початковий запит, пробіл і відповідь моделі.
        const std::string addedText = effectiveQuery.substr(query.size() + 1);

        TextProcessor processor;
        const std::vector<std::string> tokens = processor.tokenize(addedText);
        std::vector<std::string> terms;

        for (const std::string& token : tokens)
        {
            if (std::find(terms.begin(), terms.end(), token) == terms.end())
            {
                terms.push_back(token);
            }
        }

        return terms;
    }

    nlohmann::json serializeResults(const std::vector<SearchResult>& results)
    {
        nlohmann::json items = nlohmann::json::array();

        for (const SearchResult& result : results)
        {
            items.push_back({
                {"doc_id", std::to_string(result.documentId)},
                {"filename", result.filename},
                {"score", result.matchedTerms.size()},
                {"matched_terms", result.matchedTerms},
                {"snippet", result.snippet}
            });
        }

        return items;
    }
}

WebSocketController::WebSocketController(IndexService& indexService, const OllamaClient& ollamaClient)
    : indexService_(indexService),
      ollamaClient_(ollamaClient)
{
    tasks_.reserve(maxTasks_);
}

WebSocketController::~WebSocketController()
{
    stop();
}

void WebSocketController::onOpen(crow::websocket::connection& connection)
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (stopped_)
    {
        return;
    }

    std::shared_ptr<Session> session = std::make_shared<Session>();
    session->connection = &connection;

    sessions_[&connection] = std::move(session);
}

void WebSocketController::onClose(crow::websocket::connection& connection)
{
    std::lock_guard<std::mutex> lock(mutex_);

    const auto iterator = sessions_.find(&connection);

    if (iterator != sessions_.end())
    {
        iterator->second->connection = nullptr;
        sessions_.erase(iterator);
    }
}

void WebSocketController::onMessage(crow::websocket::connection& connection, const std::string& message, bool isBinary)
{
    try
    {
        if (isBinary)
        {
            connection.send_text(makeErrorMessage("Only text messages are supported.").dump());
            return;
        }

        std::string query;

        if (const std::optional<std::string> error = parseSearchMessage(message, query))
        {
            connection.send_text(makeErrorMessage(*error).dump());
            return;
        }

        std::lock_guard<std::mutex> lock(mutex_);

        const auto iterator = sessions_.find(&connection);

        if (stopped_ || iterator == sessions_.end())
        {
            return;
        }

        const std::shared_ptr<Session> session = iterator->second;

        if (session->busy)
        {
            connection.send_text(makeErrorMessage("A search is already running for this connection.").dump());
            return;
        }

        // Прибираємо завершені завдання без очікування активних.
        for (auto task = tasks_.begin(); task != tasks_.end();)
        {
            if (task->wait_for(std::chrono::seconds(0)) == std::future_status::ready)
            {
                // wait() не перекидає винятки із завершеного завдання.
                task->wait();
                task = tasks_.erase(task);
            }
            else
            {
                ++task;
            }
        }

        if (tasks_.size() >= maxTasks_)
        {
            connection.send_text(makeErrorMessage("Search server is busy. Try again later.").dump());
            return;
        }

        session->busy = true;

        try
        {
            tasks_.push_back(std::async(std::launch::async, [this, session, query]()
            {
                runSearch(session, query);
            }));
        }
        catch (...)
        {
            session->busy = false;
            throw;
        }
    }
    catch (const std::exception& error)
    {
        CROW_LOG_ERROR << "WebSocket request failed: " << error.what();
        connection.send_text(makeErrorMessage("Could not process the search request.").dump());
    }
}

bool WebSocketController::sendMessage(const std::shared_ptr<Session>& session, const nlohmann::json& message)
{
    const std::string text = message.dump();

    std::lock_guard<std::mutex> lock(mutex_);

    if (stopped_ || session->connection == nullptr)
    {
        return false;
    }

    session->connection->send_text(text);
    return true;
}

void WebSocketController::runSearch(const std::shared_ptr<Session>& session, const std::string& query)
{
    std::string errorMessage;

    try
    {
        const std::string effectiveQuery = ollamaClient_.expandQuery(query);
        const std::vector<std::string> expandedTerms = getExpandedTerms(query, effectiveQuery);

        const bool connected = sendMessage(session, {
            {"stage", "query_expansion"},
            {"data", {
                {"original", query},
                {"expanded_terms", expandedTerms}
            }}
        });

        if (connected)
        {
            const auto start = std::chrono::steady_clock::now();

            const std::vector<SearchResult> results = indexService_.search(effectiveQuery);

            const double timeMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();

            if (sendMessage(session, {
                {"stage", "keyword_search"},
                {"data", {
                    {"results", serializeResults(results)},
                    {"time_ms", timeMs}
                }}
            }))
            {
                sendMessage(session, {{"stage", "done"}});
            }
        }
    }
    catch (const std::exception& error)
    {
        CROW_LOG_ERROR << "WebSocket search failed: " << error.what();
        errorMessage = "Search pipeline failed. Check Ollama availability and server logs.";
    }
    catch (...)
    {
        CROW_LOG_ERROR << "WebSocket search failed with an unknown exception.";
        errorMessage = "Search pipeline failed.";
    }

    if (!errorMessage.empty())
    {
        try
        {
            sendMessage(session, makeErrorMessage(errorMessage));
        }
        catch (const std::exception& error)
        {
            CROW_LOG_ERROR << "WebSocket error notification failed: " << error.what();
        }
    }

    std::lock_guard<std::mutex> lock(mutex_);
    session->busy = false;
}

void WebSocketController::stop()
{
    std::vector<std::future<void>> tasks;

    {
        std::lock_guard<std::mutex> lock(mutex_);

        stopped_ = true;

        for (const auto& entry : sessions_)
        {
            entry.second->connection = nullptr;
        }

        sessions_.clear();
        tasks.swap(tasks_);
    }

    // Чекаємо поза блокуванням, щоб завдання могли завершитися.
    for (std::future<void>& task : tasks)
    {
        task.wait();
    }
}