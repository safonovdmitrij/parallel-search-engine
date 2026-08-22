#ifndef OLLAMACLIENT_H
#define OLLAMACLIENT_H

#include <string>

class OllamaClient
{
public:
    std::string generate(const std::string& prompt);
};


#endif