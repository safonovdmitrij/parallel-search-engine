#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include <exception>
#include <iomanip>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

namespace
{
    struct CurlRuntime
    {
        CurlRuntime()
        {
            if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK)
            {
                throw std::runtime_error("CURL initialization failed.");
            }
        }

        ~CurlRuntime()
        {
            curl_global_cleanup();
        }
    };

    std::size_t receiveResponse(char* data, std::size_t size, std::size_t count, void* destination) noexcept
    {
        const std::size_t bytes = size * count;
        std::string& response = *static_cast<std::string*>(destination);

        try
        {
            response.append(data, bytes);
            return bytes;
        }
        catch (...)
        {
            return 0;
        }
    }

    nlohmann::json sendSearchRequest(const std::string& query, const std::string& mode, std::string serverUrl)
    {
        std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> handle(
            curl_easy_init(),
            curl_easy_cleanup
        );

        if (!handle)
        {
            throw std::runtime_error("Could not create HTTP client.");
        }

        std::unique_ptr<curl_slist, decltype(&curl_slist_free_all)> headers(
            curl_slist_append(nullptr, "Content-Type: application/json"),
            curl_slist_free_all
        );

        if (!headers)
        {
            throw std::runtime_error("Could not create HTTP headers.");
        }

        while (!serverUrl.empty() && serverUrl.back() == '/')
        {
            serverUrl.pop_back();
        }

        const std::string url = serverUrl + "/api/search";

        const nlohmann::json request = {
            {"query", query},
            {"mode", mode}
        };

        const std::string body = request.dump();
        std::string response;

        curl_easy_setopt(handle.get(), CURLOPT_URL, url.c_str());
        curl_easy_setopt(handle.get(), CURLOPT_POST, 1L);
        curl_easy_setopt(handle.get(), CURLOPT_HTTPHEADER, headers.get());
        curl_easy_setopt(handle.get(), CURLOPT_POSTFIELDS, body.c_str());
        curl_easy_setopt(handle.get(), CURLOPT_CONNECTTIMEOUT, 5L);
        curl_easy_setopt(handle.get(), CURLOPT_TIMEOUT, 150L);
        curl_easy_setopt(handle.get(), CURLOPT_NOSIGNAL, 1L);
        curl_easy_setopt(handle.get(), CURLOPT_WRITEFUNCTION, receiveResponse);
        curl_easy_setopt(handle.get(), CURLOPT_WRITEDATA, &response);

        const CURLcode result = curl_easy_perform(handle.get());

        if (result != CURLE_OK)
        {
            throw std::runtime_error(
                std::string("HTTP request failed: ") + curl_easy_strerror(result)
            );
        }

        long status = 0;
        curl_easy_getinfo(handle.get(), CURLINFO_RESPONSE_CODE, &status);

        const nlohmann::json message = nlohmann::json::parse(response, nullptr, false);

        if (!message.is_object())
        {
            throw std::runtime_error(
                "Server returned invalid JSON. HTTP status: " + std::to_string(status)
            );
        }

        if (status != 200)
        {
            throw std::runtime_error(
                "HTTP " + std::to_string(status) + ": "
                + message.value("error", std::string("Search failed."))
            );
        }

        return message;
    }

    void printResults(const nlohmann::json& response)
    {
        std::cout << "Query: " << response.at("query").get<std::string>() << '\n';
        std::cout << "Mode: " << response.at("mode").get<std::string>() << '\n';

        if (response.at("mode") == "llm")
        {
            std::cout << "Expanded query: "
                      << response.at("expanded_query").get<std::string>()
                      << '\n';
        }

        const nlohmann::json& results = response.at("results");

        std::cout << "Documents found: " << results.size() << '\n';

        std::cout << "Search time: "
                  << std::fixed << std::setprecision(2)
                  << response.at("time_ms").get<double>()
                  << " ms\n\n";

        for (const nlohmann::json& result : results)
        {
            const std::string documentId = result.at("doc_id").get<std::string>();
            std::string filename = result.value("filename", std::string());

            if (filename.empty())
            {
                filename = "Document " + documentId;
            }

            std::cout << filename << " [id: " << documentId << "]\n";
            std::cout << "Score: " << result.at("score") << '\n';

            std::cout << "Matched terms:";

            for (const nlohmann::json& term : result.at("matched_terms"))
            {
                std::cout << ' ' << term.get<std::string>();
            }

            std::cout << '\n';
            std::cout << result.at("snippet").get<std::string>() << "\n\n";
        }
    }
}

int main(int argc, char* argv[])
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    if (argc < 2 || argc > 4)
    {
        std::cerr << "Usage: search \"query\" [keyword|llm] [server_url]\n";
        return 1;
    }

    try
    {
        const std::string query = argv[1];
        const std::string mode = argc >= 3 ? argv[2] : "keyword";
        const std::string serverUrl = argc >= 4 ? argv[3] : "http://127.0.0.1:8080";

        if (query.size() > 1000 || query.find_first_not_of(" \t\r\n") == std::string::npos)
        {
            throw std::invalid_argument("Query must contain text and not exceed 1000 bytes.");
        }

        if (mode != "keyword" && mode != "llm")
        {
            throw std::invalid_argument("Mode must be 'keyword' or 'llm'.");
        }

        CurlRuntime runtime;

        const nlohmann::json response = sendSearchRequest(query, mode, serverUrl);

        printResults(response);
    }
    catch (const std::exception& error)
    {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }

    return 0;
}