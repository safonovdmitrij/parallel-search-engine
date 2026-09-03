#ifndef OLLAMACLIENT_H
#define OLLAMACLIENT_H

#include <string>

class OllamaClient
{
public:
    std::string expandQuery(const std::string& query);
};


#endif