#include <chrono>
#include <exception>
#include <iostream>

#include "DataLoader.h"
#include "TextProcessor.h"
#include "ApiController.h"
#include "Server.h"
#include <stdexcept>
#include "IndexService.h"
#include "OllamaClient.h"
#include "WebSocketController.h"


int main()
{
    try
    {
        DataLoader dataLoader("data/test");
        TextProcessor textProcessor;

        IndexService indexService(
            dataLoader,
            textProcessor
        );

        const auto statistics = indexService.rebuild(4);

        if (!statistics)
        {
            throw std::runtime_error(
                "Initial index build could not start."
            );
        }

        std::cout
            << "Index built.\n"
            << "Documents indexed: "
            << statistics->documents << '\n'
            << "Build time: "
            << statistics->timeMs << " ms\n";

        OllamaClient ollamaClient(
    "http://127.0.0.1:11434",
    "gemma3:1b"
        );

        ApiController apiController(
            indexService,
            ollamaClient
        );

        WebSocketController webSocketController(indexService, ollamaClient);

        Server server(apiController, webSocketController);

        server.run(8080, 4);
    }
    catch (const std::exception& error)
    {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }

    return 0;
}