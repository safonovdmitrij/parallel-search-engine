#include <chrono>
#include <exception>
#include <iostream>
#include <string>

#include "DataLoader.h"
#include "TextProcessor.h"
#include "InvertedIndex.h"
#include "Indexer.h"
#include "SearchService.h"

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

        const int threadCount = 4;

        const auto start = std::chrono::steady_clock::now();

        const auto documentPaths = indexer.build(threadCount);

        const auto finish = std::chrono::steady_clock::now();

        const double timeMs =
            std::chrono::duration<double, std::milli>(
                finish - start
            ).count();

        std::cout << "Index built.\n"
                  << "Threads: " << threadCount << '\n'
                  << "Files found: " << documentPaths.size() << '\n'
                  << "Build time: " << timeMs << " ms\n\n";

        for (DocumentId documentId = 0;
             documentId < documentPaths.size();
             ++documentId)
        {
            std::cout << "Document ID: " << documentId
                      << " | Path: "
                      << documentPaths[documentId].string()
                      << '\n';
        }

        SearchService searchService(
            invertedIndex,
            textProcessor
        );

        std::cout << "\nEnter a single-term query."
                  << "\nEnter 'exit' to quit.\n";

        std::string query;

        while (std::cout << "\nQuery: "
               && std::getline(std::cin, query))
        {
            if (query == "exit")
            {
                break;
            }

            try
            {
                const auto postings = searchService.search(query);

                std::cout << "Documents found: "
                          << postings.size() << '\n';

                for (const Posting& posting : postings)
                {
                    std::cout << "Document ID: "
                              << posting.documentId
                              << " | Path: "
                              << documentPaths.at(
                                     posting.documentId
                                 ).string()
                              << " | Positions:";

                    for (std::size_t position : posting.positions)
                    {
                        std::cout << ' ' << position;
                    }

                    std::cout << '\n';
                }
            }
            catch (const std::exception& exception)
            {
                std::cerr << "Search error: "
                          << exception.what() << '\n';
            }
        }
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Error: " << exception.what() << '\n';
        return 1;
    }

    return 0;
}