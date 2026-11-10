#include "ApiController.h"

#include <exception>
#include <string>
#include <utility>
#include <nlohmann/json.hpp>

ApiController::ApiController(IndexService& indexService)
    : indexService_(indexService)
{
}

crow::response ApiController::health() const
{
    crow::json::wvalue body;
    body["status"] = "ok";

    return crow::response(body);
}

crow::response ApiController::search(const crow::request& request) const
{
    // Отримуємо параметр q з адреси запиту.
    const char* queryParameter = request.url_params.get("q");

    if (queryParameter == nullptr || std::string(queryParameter).empty())
    {
        crow::json::wvalue body;
        body["error"] = "Query parameter 'q' is required.";

        crow::response response(body);
        response.code = 400;

        return response;
    }

    const std::string query(queryParameter);

    if (query.size() > 1000)
    {
        crow::json::wvalue body;
        body["error"] = "Query must not exceed 1000 bytes.";

        crow::response response(body);
        response.code = 400;

        return response;
    }

    try
    {
        const auto results = indexService_.search(query);

        crow::json::wvalue body;
        body["query"] = query;
        body["total"] = results.size();
        body["results"] = crow::json::wvalue::list();

        for (std::size_t i = 0; i < results.size(); ++i)
        {
            const SearchResult& result = results[i];

            crow::json::wvalue item;
            item["doc_id"] = result.documentId;
            item["score"] = result.matchedTerms.size();
            item["matched_terms"] = result.matchedTerms;

            body["results"][i] = std::move(item);
        }

        return crow::response(body);
    }
    catch (const std::exception& error)
    {
        CROW_LOG_ERROR << "Search failed: " << error.what();

        crow::json::wvalue body;
        body["error"] = "Search failed.";

        crow::response response(body);
        response.code = 500;

        return response;
    }
}


crow::response ApiController::index(const crow::request& request) const
{
    // Допоміжна функція для JSON-відповідей із помилкою.
    const auto errorResponse = [](
        int status,
        const std::string& message
    ) -> crow::response
    {
        crow::json::wvalue body;
        body["error"] = message;

        crow::response response(body);
        response.code = status;

        return response;
    };

    try
    {
        const auto input = nlohmann::json::parse(
            request.body,
            nullptr,
            false
        );

        if (!input.is_object()
            || !input.contains("threads")
            || !input["threads"].is_number_integer())
        {
            return errorResponse(
                400,
                "JSON field 'threads' must be an integer."
            );
        }

        // Перевіряємо межі перед перетворенням на int.
        if (input["threads"] < 1 || input["threads"] > 128)
        {
            return errorResponse(
                400,
                "Thread count must be between 1 and 128."
            );
        }

        const int threadCount = input["threads"].get<int>();

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
        CROW_LOG_ERROR << "Index rebuild failed: " << error.what();

        return errorResponse(
            500,
            "Index rebuild failed. Previous index is unchanged."
        );
    }
}