#include "Server.h"

#include <cstdint>
#include <stdexcept>

Server::Server(const ApiController& apiController, WebSocketController& webSocketController)
    : apiController_(apiController),
      webSocketController_(webSocketController)
{
    CROW_ROUTE(app_, "/api/health")([this]()
    {
        return apiController_.health();
    });

    CROW_ROUTE(app_, "/api/stats")([this]()
    {
        return apiController_.stats();
    });

    CROW_ROUTE(app_, "/api/search")
        .methods(crow::HTTPMethod::GET, crow::HTTPMethod::POST)
        ([this](const crow::request& request)
        {
            return apiController_.search(request);
        });

    CROW_ROUTE(app_, "/api/index")
        .methods(crow::HTTPMethod::POST)
        ([this](const crow::request& request)
        {
            return apiController_.index(request);
        });

    CROW_WEBSOCKET_ROUTE(app_, "/ws/search")
        .onopen([this](crow::websocket::connection& connection)
        {
            webSocketController_.onOpen(connection);
        })
        .onmessage([this](crow::websocket::connection& connection, const std::string& message, bool isBinary)
        {
            webSocketController_.onMessage(connection, message, isBinary);
        })
        .onclose([this](crow::websocket::connection& connection, const std::string&, auto...)
        {
            webSocketController_.onClose(connection);
        });
}

Server::~Server()
{
    // Завершуємо фонові завдання до знищення app_.
    webSocketController_.stop();
}

void Server::run(int port, int threadCount)
{
    if (port < 1 || port > 65535)
    {
        throw std::invalid_argument("Port must be between 1 and 65535.");
    }

    if (threadCount < 2 || threadCount > 128)
    {
        throw std::invalid_argument("Server thread count must be between 2 and 128.");
    }

    app_.bindaddr("127.0.0.1")
        .port(static_cast<std::uint16_t>(port))
        .concurrency(static_cast<unsigned int>(threadCount))
        .run();
}