#ifndef OLLAMACLIENT_H
#define OLLAMACLIENT_H

#include <string>

class OllamaClient
{
public:
    explicit OllamaClient(
        std::string baseUrl = "http://127.0.0.1:11434",
        std::string model = "gemma3:1b"
    );

    bool isAvailable() const;

    std::string expandQuery(const std::string& query) const;

private:
    std::string baseUrl_;
    std::string model_;

    std::string request(
        const std::string& path,
        const std::string& body,
        long timeout
    ) const;
};

#endif
