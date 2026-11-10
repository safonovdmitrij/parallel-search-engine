#include <chrono>
#include <exception>
#include <iostream>

#include "DataLoader.h"
#include "TextProcessor.h"
#include "InvertedIndex.h"
#include "Indexer.h"
#include "SearchService.h"
#include "ApiController.h"
#include "Server.h"

int main()
{
    try
    {
        DataLoader dataLoader("data/test");
        TextProcessor textProcessor;
        InvertedIndex invertedIndex;

        Indexer indexer(
            dataLoader,
            textProcessor,
            invertedIndex
        );

        const int indexingThreadCount = 4;
        const int serverThreadCount = 4;
        const int port = 8080;

        const auto start = std::chrono::steady_clock::now();

        const auto documentPaths =
            indexer.build(indexingThreadCount);

        const auto finish = std::chrono::steady_clock::now();

        const double timeMs =
            std::chrono::duration<double, std::milli>(
                finish - start
            ).count();

        std::cout << "Index built.\n"
            << "Indexing threads: "
            << indexingThreadCount << '\n'
            << "Files found: "
            << documentPaths.size() << '\n'
            << "Build time: "
            << timeMs << " ms\n\n";

        SearchService searchService(
            invertedIndex,
            textProcessor
        );

        ApiController apiController(searchService);
        Server server(apiController);

        server.run(port, serverThreadCount);
    }
    catch (const std::exception& error)
    {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }

    return 0;
}
