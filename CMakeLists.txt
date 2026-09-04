cmake_minimum_required(VERSION 3.29)
project(parallel_search_engine)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(CURL REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

add_executable(search_server
        src/main.cpp
        src/OllamaClient.cpp
        src/DataLoader.cpp
        src/TextProcessor.cpp
)

target_link_libraries(search_server PRIVATE
        CURL::libcurl
        nlohmann_json::nlohmann_json
)