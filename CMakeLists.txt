cmake_minimum_required(VERSION 3.29)
project(parallel_search_engine)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(CURL REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Threads REQUIRED)

include(FetchContent)

set(CROW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(CROW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(CROW_ENABLE_SSL OFF CACHE BOOL "" FORCE)
set(CROW_ENABLE_COMPRESSION OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
        Crow
        GIT_REPOSITORY https://github.com/CrowCpp/Crow.git
        GIT_TAG v1.3.4
        GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(Crow)

add_executable(search_server
        src/main.cpp
        src/OllamaClient.cpp
        src/DataLoader.cpp
        src/TextProcessor.cpp
        src/InvertedIndex.cpp
        src/Indexer.cpp
        src/IndexService.cpp
        src/SearchService.cpp
        src/ThreadPool.cpp
        src/ApiController.cpp
        src/Server.cpp
        src/WebSocketController.cpp
)

add_executable(search cli/main.cpp)

target_link_libraries(search_server PRIVATE
        CURL::libcurl
        nlohmann_json::nlohmann_json
        Crow::Crow
        Threads::Threads
)

target_link_libraries(search PRIVATE
        CURL::libcurl
        nlohmann_json::nlohmann_json
)

if(WIN32)
    target_link_libraries(search_server PRIVATE
            ws2_32
            mswsock
    )

endif()

find_package(Threads REQUIRED)

add_executable(index_benchmark
        scripts/tests/index_benchmark.cpp
        src/DataLoader.cpp
        src/TextProcessor.cpp
        src/InvertedIndex.cpp
        src/Indexer.cpp
        src/ThreadPool.cpp
)

target_include_directories(index_benchmark PRIVATE src)

target_link_libraries(index_benchmark PRIVATE Threads::Threads)