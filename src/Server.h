#ifndef SERVER_H
#define SERVER_H

#include <crow.h>

#include "ApiController.h"
#include "WebSocketController.h"

class Server
{
public:
    Server(const ApiController& apiController, WebSocketController& webSocketController);
    ~Server();

    void run(int port, int threadCount);

private:
    const ApiController& apiController_;
    WebSocketController& webSocketController_;
    crow::SimpleApp app_;
};

#endif //SERVER_H
