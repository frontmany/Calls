# syntax=docker/dockerfile:1

FROM ubuntu:22.04 AS builder

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential cmake git ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Vendor deps (same tags as root CMakeLists FetchContent). Use when vendor/ is not in git.
RUN mkdir -p /app/vendor \
    && git clone --depth 1 --branch v3.11.3 https://github.com/nlohmann/json.git /app/vendor/json \
    && git clone --depth 1 --branch asio-1-30-2 https://github.com/chriskohlhoff/asio.git /app/vendor/asio \
    && git clone --depth 1 --branch v1.14.1 https://github.com/gabime/spdlog.git /app/vendor/spdlog \
    && test -f /app/vendor/json/include/nlohmann/json.hpp \
    && test -f /app/vendor/asio/asio/include/asio.hpp \
    && test -f /app/vendor/spdlog/CMakeLists.txt

COPY serverUpdater /app/serverUpdater

# Build static dependency expected by `serverUpdater/CMakeLists.txt` (Linux branch).
RUN cd /app/vendor/spdlog \
    && rm -rf build \
    && cmake -S . -B build \
        -DCMAKE_BUILD_TYPE=Release \
        -DSPDLOG_BUILD_SHARED=OFF \
        -DSPDLOG_BUILD_TESTS=OFF \
        -DSPDLOG_BUILD_EXAMPLE=OFF \
    && cmake --build build -j"$(nproc)" \
    && ( \
        if [ -f build/libspdlog.a ]; then cp -f build/libspdlog.a /app/vendor/spdlog/libspdlog.a; \
        elif [ -f build/Release/libspdlog.a ]; then cp -f build/Release/libspdlog.a /app/vendor/spdlog/libspdlog.a; \
        else echo "libspdlog.a not found in spdlog build output" && exit 1; \
        fi \
    )

RUN mkdir -p /app/bin \
    && cmake -S serverUpdater -B build-updater -DCMAKE_BUILD_TYPE=Release -DCMAKE_RUNTIME_OUTPUT_DIRECTORY=/app/bin \
    && cmake --build build-updater -j"$(nproc)"


FROM ubuntu:22.04 AS runtime

RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

RUN mkdir -p logs versions

COPY --from=builder /app/bin/calliforniaServerUpdater /app/calliforniaServerUpdater
RUN chmod +x /app/calliforniaServerUpdater

EXPOSE 8082/tcp

ENTRYPOINT ["/app/calliforniaServerUpdater"]

