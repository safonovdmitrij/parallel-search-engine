FROM debian:trixie-slim

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        g++ \
        cmake \
        ninja-build \
        git \
        ca-certificates \
        libasio-dev \
        libcurl4-openssl-dev \
        nlohmann-json3-dev \
        curl \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY CMakeLists.txt ./
COPY src ./src
COPY cli ./cli
COPY web ./web

RUN cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build --parallel 2 --target search_server search

EXPOSE 8080

CMD ["/app/build/search_server"]