#include "DataLoader.h"
#include "Indexer.h"
#include "InvertedIndex.h"
#include "TextProcessor.h"

#include <chrono>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>

int main(int argc, char* argv[])
{
    try
    {
        if (argc != 3)
        {
            throw std::invalid_argument(
                "Usage: index_benchmark <data_directory> <threads>"
            );
        }

        std::size_t parsed = 0;
        const std::string argument = argv[2];
        const int threads = std::stoi(argument, &parsed);

        if (parsed != argument.size() || threads < 1 || threads > 128)
        {
            throw std::invalid_argument(
                "Threads must be an integer between 1 and 128."
            );
        }

        const std::filesystem::path directory = argv[1];

        if (!std::filesystem::is_directory(directory))
        {
            throw std::invalid_argument(
                "Dataset directory does not exist."
            );
        }

        DataLoader loader(directory);
        TextProcessor processor;
        InvertedIndex index;
        Indexer indexer(loader, processor, index);

        const auto start = std::chrono::steady_clock::now();

        const auto paths = indexer.build(threads);

        const auto finish = std::chrono::steady_clock::now();

        const auto documents = index.getDocumentsCount();

        if (documents == 0 || documents != paths.size())
        {
            throw std::runtime_error(
                "Dataset is empty or some documents were not indexed."
            );
        }

        const double timeMs =
            std::chrono::duration<double, std::milli>(
                finish - start
            ).count();

        std::cout << std::fixed << std::setprecision(6)
                  << "{\"threads\":" << threads
                  << ",\"documents\":" << documents
                  << ",\"time_ms\":" << timeMs
                  << "}\n";

        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "Benchmark failed: "
                  << error.what() << '\n';

        return 1;
    }
}