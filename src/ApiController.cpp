#include "ApiController.h"

#include <nlohmann/json.hpp>

#include <chrono>
#include <exception>
#include <string>
#include <utility>
#include <optional>

namespace
{
    crow::response errorResponse(int status, const std::string& message)
    {
        crow::json::wvalue body;
        body["error"] = message;

        crow::response response(body);
        response.code = status;

        return response;
    }

    struct SearchParameters
    {
        std::string query;
        std::string mode = "keyword";
    };

    std::optional<std::string> parseSearchRequest(const crow::request& request,SearchParameters& parameters)
    {
        if (request.method == crow::HTTPMethod::POST)
        {
            const auto input = nlohmann::json::parse(request.body, nullptr, false);

            if (!input.is_object() || !input.contains("query") || !input["query"].is_string())
            {
                return "JSON field 'query' must be a string.";
            }

            parameters.query = input["query"].get<std::string>();

            if (input.contains("mode"))
            {
                if (!input["mode"].is_string())
                {
                    return "Field 'mode' must be a string.";
                }

                parameters.mode = input["mode"].get<std::string>();
            }
        }
        else
        {
            const char* query = request.url_params.get("q");

            if (query == nullptr)
            {
                return "Query parameter 'q' is required.";
            }

            parameters.query = query;

            if (const char* mode = request.url_params.get("mode"))
            {
                parameters.mode = mode;
            }
        }

        if (parameters.query.size() > 1000 || parameters.query.find_first_not_of(" \t\r\n") == std::string::npos)
        {
            return "Query must contain text and not exceed 1000 bytes.";
        }

        if (parameters.mode != "keyword" && parameters.mode != "llm")
        {
            return "Mode must be 'keyword' or 'llm'.";
        }

        return std::nullopt;
    }

    std::optional<std::string> parseIndexRequest(const crow::request& request,int& threadCount)
    {
        const auto input = nlohmann::json::parse(request.body, nullptr, false);

        if (!input.is_object() || !input.contains("threads") || !input["threads"].is_number_integer())
        {
            return "JSON field 'threads' must be an integer.";
        }

        if (input["threads"] < 1 || input["threads"] > 128)
        {
            return "Thread count must be between 1 and 128.";
        }

        threadCount = input["threads"].get<int>();

        return std::nullopt;
    }
}

ApiController::ApiController(IndexService& indexService, const OllamaClient& ollamaClient)
    : indexService_(indexService),
      ollamaClient_(ollamaClient)
{
}

crow::response ApiController::health() const
{
    crow::json::wvalue body;
    body["status"] = "ok";
    body["ollama"] = ollamaClient_.isAvailable();

    return crow::response(body);
}

crow::response ApiController::stats() const
{
    const auto statistics = indexService_.getStatistics();

    crow::json::wvalue body;

    body["documents"] = statistics.documents;
    body["index_build_time_ms"] = statistics.timeMs;
    body["threads_used"] = statistics.threadsUsed;

    return crow::response(body);
}

crow::response ApiController::search(const crow::request& request) const
{
    try
    {
        SearchParameters parameters;

        if (const auto error = parseSearchRequest(request, parameters))
        {
            return errorResponse(400, *error);
        }

        const std::string& query = parameters.query;
        const std::string& mode = parameters.mode;

        const auto start = std::chrono::steady_clock::now();

        std::string effectiveQuery = query;

        if (mode == "llm")
        {
            try
            {
                effectiveQuery = ollamaClient_.expandQuery(query);
            }
            catch (const std::exception& error)
            {
                CROW_LOG_ERROR
                    << "Query expansion failed: "
                    << error.what();

                return errorResponse(
                    503,
                    "Ollama query expansion is unavailable."
                );
            }
        }

        const std::vector<SearchResult> results = indexService_.search(effectiveQuery);

        const double timeMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();

        crow::json::wvalue body;

        body["query"] = query;
        body["mode"] = mode;
        body["expanded_query"] = effectiveQuery;
        body["time_ms"] = timeMs;
        body["total"] = results.size();
        body["results"] = crow::json::wvalue::list();

        for (std::size_t i = 0; i < results.size(); ++i)
        {
            const auto& result = results[i];

            crow::json::wvalue item;

            item["doc_id"] = std::to_string(result.documentId);

            item["score"] = result.matchedTerms.size();
            item["matched_terms"] = result.matchedTerms;
            item["snippet"] = result.snippet;

            body["results"][i] = std::move(item);
        }

        return crow::response(body);
    }
    catch (const std::exception& error)
    {
        CROW_LOG_ERROR
            << "Search failed: "
            << error.what();

        return errorResponse(500, "Search failed.");
    }
}


crow::response ApiController::index(const crow::request& request) const
{
    try
    {
        int threadCount = 0;

        if (const auto error = parseIndexRequest(request, threadCount))
        {
            return errorResponse(400, *error);
        }

        const auto statistics = indexService_.rebuild(threadCount);

        if (!statistics)
        {
            return errorResponse(
                409,
                "Index rebuild is already in progress."
            );
        }

        crow::json::wvalue body;
        body["status"] = "ok";
        body["documents_indexed"] = statistics->documents;
        body["time_ms"] = statistics->timeMs;

        return crow::response(body);
    }
    catch (const std::exception& error)
    {
        CROW_LOG_ERROR
            << "Index rebuild failed: "
            << error.what();

        return errorResponse(
            500,
            "Index rebuild failed. Previous index is unchanged."
        );
    }
}