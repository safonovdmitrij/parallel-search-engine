#ifndef SERVER_H
#define SERVER_H

#include <crow.h>

#include "ApiController.h"

class Server
{
public:
    explicit Server(const ApiController& apiController);

    void run(int port, int threadCount);

private:
    const ApiController& apiController_;
    crow::SimpleApp app_;
};

#endif //SERVER_H
