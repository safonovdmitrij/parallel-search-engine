#ifndef WEBSOCKETCONTROLLER_H
#define WEBSOCKETCONTROLLER_H

#include <crow.h>
#include <nlohmann/json.hpp>

#include <future>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "IndexService.h"
#include "OllamaClient.h"

class WebSocketController
{
public:
    WebSocketController(IndexService& indexService, const OllamaClient& ollamaClient);
    ~WebSocketController();

    void onOpen(crow::websocket::connection& connection);
    void onClose(crow::websocket::connection& connection);
    void onMessage(crow::websocket::connection& connection, const std::string& message, bool isBinary);

    void stop();

private:
    struct Session
    {
        crow::websocket::connection* connection = nullptr;
        bool busy = false;
    };

    void runSearch(const std::shared_ptr<Session>& session, const std::string& query);
    bool sendMessage(const std::shared_ptr<Session>& session, const nlohmann::json& message);

    IndexService& indexService_;
    const OllamaClient& ollamaClient_;

    std::mutex mutex_;
    std::unordered_map<crow::websocket::connection*, std::shared_ptr<Session>> sessions_;
    std::vector<std::future<void>> tasks_;

    bool stopped_ = false;

    static constexpr std::size_t maxTasks_ = 4;
};

#endif //WEBSOCKETCONTROLLER_H
