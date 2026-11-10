#ifndef APICONTROLLER_H
#define APICONTROLLER_H

#include <crow.h>

#include "IndexService.h"
#include "OllamaClient.h"

class ApiController
{
public:
    explicit ApiController(IndexService& indexService, const OllamaClient& ollamaClient);

    crow::response health() const;
    crow::response stats() const;
    crow::response search(const crow::request& request) const;
    crow::response index(const crow::request& request) const;

private:
    IndexService& indexService_;
    const OllamaClient& ollamaClient_;
};


#endif //APICONTROLLER_H
