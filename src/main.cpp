#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include <iostream>
#include <string>

int main()
{
    CURLcode result, initResult;
    CURL* curl;

    initResult = curl_global_init(CURL_GLOBAL_DEFAULT);
    if(initResult != CURLE_OK)
    {
        std::cerr << "Curl global initialization failed: " << curl_easy_strerror(initResult) << std::endl;
        return -1;
    }

    curl = curl_easy_init();
    if(curl == nullptr)
    {
        std::cerr << "Eazy handle initialization failed: " << std::endl;
        curl_global_cleanup();
        return -1;
    }

    curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:11434/api/generate");

    const nlohmann::json requestJson = {
        {"model", "gemma3:1b"},
        {"prompt", "Why is the sky blue?"},
        {"stream", false}
    };

    const std::string requestBody = requestJson.dump();

    curl_slist* headers = nullptr;

    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, requestBody.c_str());

    result = curl_easy_perform(curl);
    if(result != CURLE_OK)
    {
        std::cerr << "Curl perform failed: " << curl_easy_strerror(result) << std::endl;
        return -1;
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    curl_global_cleanup();
    return 0;
}
