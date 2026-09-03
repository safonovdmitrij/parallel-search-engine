#include "OllamaClient.h"

#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include <iostream>
#include <string>

std::size_t writeCallback(char* data, std::size_t size, std::size_t count, void* userData)
{
    const std::size_t totalSize = size * count;

    auto* responseBody = static_cast<std::string*>(userData);

    responseBody->append(data, totalSize);

    return totalSize;
}


std::string OllamaClient::expandQuery(const std::string& query)
{
    CURLcode result, initResult;
    CURL* curl;
    std::string requestBody, responseBody;

    initResult = curl_global_init(CURL_GLOBAL_DEFAULT);
    if(initResult != CURLE_OK)
    {
        std::cerr << "Curl global initialization failed: " << curl_easy_strerror(initResult) << std::endl;
        return "";
    }

    curl = curl_easy_init();
    if(curl == nullptr)
    {
        std::cerr << "Eazy handle initialization failed: " << std::endl;
        curl_global_cleanup();
        return "";
    }

    curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:11434/api/generate");

    const std::string expansionPrompt =
        "Expand the following search query with relevant synonyms and related terms. "
        "Return only the expanded query without explanations.\n"
        "Query: " + query;


    const nlohmann::json requestJson = {
        {"model", "gemma3:1b"},
        {"prompt", expansionPrompt},
        {"stream", false}
    };

    requestBody = requestJson.dump();

    curl_slist* headers = nullptr;

    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, requestBody.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBody);

    result = curl_easy_perform(curl);
    if(result != CURLE_OK)
    {
        std::cerr << "Curl perform failed: " << curl_easy_strerror(result) << std::endl;

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        curl_global_cleanup();

        return "";
    }

    nlohmann::json ollamaResponse = nlohmann::json::parse(responseBody);
    std::string modelResponse = ollamaResponse.at("response").get<std::string>();

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    curl_global_cleanup();

    return modelResponse;
}