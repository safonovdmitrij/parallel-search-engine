#ifndef APICONTROLLER_H
#define APICONTROLLER_H

#include <crow.h>

#include "IndexService.h"

class ApiController
{
public:
    explicit ApiController(IndexService& indexService);

    crow::response health() const;
    crow::response search(const crow::request& request) const;
    crow::response index(const crow::request& request) const;

private:
    IndexService& indexService_;
};


#endif //APICONTROLLER_H
