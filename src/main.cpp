#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include <iostream>

int main()
{
    const curl_version_info_data* curlInfo = curl_version_info(CURLVERSION_NOW);
    nlohmann::json result = {
        {"status", "ok"},
        {"curl_version", curlInfo->version},
        {"json_library", "nlohmann-json"}
    };
    std::cout << result.dump(4) << std::endl;
    return 0;
}
