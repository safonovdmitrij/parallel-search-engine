#include <chrono>
#include <exception>
#include <iostream>
#include <cstdlib>

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
        const char* dataDirectory = std::getenv("DATA_DIR");

        DataLoader dataLoader(dataDirectory ? dataDirectory : "data/raw");

        TextProcessor textProcessor;

        IndexService indexService(
            dataLoader,
            textProcessor
        );

        const auto statistics = indexService.rebuild(2);

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

        const char* ollamaUrl = std::getenv("OLLAMA_URL");
        const char* ollamaModel = std::getenv("OLLAMA_MODEL");

        OllamaClient ollamaClient(
            ollamaUrl ? ollamaUrl : "http://127.0.0.1:11434",
            ollamaModel ? ollamaModel : "gemma3:1b"
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