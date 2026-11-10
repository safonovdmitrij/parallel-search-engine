#include <chrono>
#include <exception>
#include <iostream>

#include "DataLoader.h"
#include "TextProcessor.h"
#include "ApiController.h"
#include "Server.h"
#include <stdexcept>
#include "IndexService.h"


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

        ApiController apiController(indexService);
        Server server(apiController);

        server.run(8080, 4);
    }
    catch (const std::exception& error)
    {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }

    return 0;
}