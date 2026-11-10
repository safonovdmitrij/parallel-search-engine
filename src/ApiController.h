#ifndef APICONTROLLER_H
#define APICONTROLLER_H

#include <crow.h>

#include "SearchService.h"

class ApiController
{
public:
    explicit ApiController(const SearchService& searchService);

    crow::response health() const;
    crow::response search(const crow::request& request) const;

private:
    const SearchService& searchService_;
};


#endif //APICONTROLLER_H
