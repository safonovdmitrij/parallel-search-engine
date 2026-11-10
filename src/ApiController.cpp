#include "ApiController.h"

#include <exception>
#include <string>
#include <utility>

ApiController::ApiController(const SearchService& searchService)
    : searchService_(searchService)
{
}

crow::response ApiController::health() const
{
    crow::json::wvalue body;
    body["status"] = "ok";

    return crow::response(body);
}

crow::response ApiController::search(
    const crow::request& request
) const
{
    // Отримуємо параметр q з адреси запиту.
    const char* queryParameter = request.url_params.get("q");

    if (queryParameter == nullptr
        || std::string(queryParameter).empty())
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
        const auto results = searchService_.search(query);

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