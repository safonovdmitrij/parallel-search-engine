#include "OllamaClient.h"

#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include <memory>
#include <stdexcept>
#include <utility>

namespace
{
    struct CurlRuntime
    {
        CurlRuntime()
        {
            if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK)
            {
                throw std::runtime_error(
                    "CURL initialization failed."
                );
            }
        }

        ~CurlRuntime()
        {
            curl_global_cleanup();
        }
    };

    std::size_t writeBody(
        char* data,
        std::size_t size,
        std::size_t count,
        void* destination
    ) noexcept
    {
        const std::size_t bytes = size * count;

        auto& body = *static_cast<std::string*>(destination);

        // Обмежуємо відповідь одним мегабайтом.
        if (bytes > 1024 * 1024 - body.size())
        {
            return 0;
        }

        try
        {
            body.append(data, bytes);
        }
        catch (...)
        {
            return 0;
        }

        return bytes;
    }
}

OllamaClient::OllamaClient(
    std::string baseUrl,
    std::string model
)
    : baseUrl_(std::move(baseUrl)),
      model_(std::move(model))
{
    // Ініціалізація виконується один раз.
    static CurlRuntime runtime;
}

std::string OllamaClient::request(
    const std::string& path,
    const std::string& body,
    long timeout
) const
{
    std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> handle(
        curl_easy_init(),
        curl_easy_cleanup
    );

    if (!handle)
    {
        throw std::runtime_error(
            "CURL handle creation failed."
        );
    }

    std::unique_ptr<
        curl_slist,
        decltype(&curl_slist_free_all)
    > headers(
        curl_slist_append(
            nullptr,
            "Content-Type: application/json"
        ),
        curl_slist_free_all
    );

    if (!headers)
    {
        throw std::runtime_error(
            "CURL headers creation failed."
        );
    }

    const std::string url = baseUrl_ + path;
    std::string response;

    curl_easy_setopt(
        handle.get(), CURLOPT_URL, url.c_str()
    );

    curl_easy_setopt(
        handle.get(), CURLOPT_HTTPHEADER, headers.get()
    );

    curl_easy_setopt(
        handle.get(), CURLOPT_CONNECTTIMEOUT, 2L
    );

    curl_easy_setopt(
        handle.get(), CURLOPT_TIMEOUT, timeout
    );

    curl_easy_setopt(
        handle.get(), CURLOPT_NOSIGNAL, 1L
    );

    curl_easy_setopt(
        handle.get(), CURLOPT_WRITEFUNCTION, writeBody
    );

    curl_easy_setopt(
        handle.get(), CURLOPT_WRITEDATA, &response
    );

    if (!body.empty())
    {
        curl_easy_setopt(
            handle.get(),
            CURLOPT_POSTFIELDS,
            body.c_str()
        );
    }

    const CURLcode result = curl_easy_perform(handle.get());

    if (result != CURLE_OK)
    {
        throw std::runtime_error(
            curl_easy_strerror(result)
        );
    }

    long status = 0;

    curl_easy_getinfo(
        handle.get(),
        CURLINFO_RESPONSE_CODE,
        &status
    );

    if (status != 200)
    {
        throw std::runtime_error(
            "Ollama HTTP status: " + std::to_string(status)
        );
    }

    return response;
}

bool OllamaClient::isAvailable() const
{
    try
    {
        const auto data = nlohmann::json::parse(
            request("/api/tags", "", 2L)
        );

        for (const auto& item : data.at("models"))
        {
            if (item.value("name", "") == model_)
            {
                return true;
            }
        }
    }
    catch (const std::exception&)
    {
        return false;
    }

    return false;
}

std::string OllamaClient::expandQuery(const std::string& query) const
{
    const nlohmann::json input = {
        {"model", model_},
        {
            "system",
            "You expand search queries for a document search engine. "
            "Return 3 to 8 relevant English synonyms or closely related "
            "terms for the given query. "
            "Return only the terms separated by spaces. "
            "Do not acknowledge the request or provide explanations.\n"
            "Example:\n"
            "Input: computer\n"
            "Output: computing processor hardware software PC"
        },
        {"prompt", "Expand this search query: " + query},
        {"stream", false},
        {
            "options",
            {
                {"temperature", 0},
                {"num_predict", 64}
            }
        }
    };

    const auto data = nlohmann::json::parse(request("/api/generate", input.dump(), 120L));

    const std::string expanded = data.at("response").get<std::string>();

    if (expanded.find_first_not_of(" \t\r\n") == std::string::npos)
    {
        throw std::runtime_error("Ollama returned an empty query.");
    }

    // Початкові слова також залишаються в пошуку.
    return query + " " + expanded;
}
