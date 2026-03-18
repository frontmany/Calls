# syntax=docker/dockerfile:1

FROM ubuntu:22.04 AS builder

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential cmake git ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY vendor /app/vendor
COPY server /app/server

# Build static dependencies expected by `server/CMakeLists.txt` (Linux branch).
# `server/CMakeLists.txt` links by file names: `libcryptopp.a` and `libspdlog.a`
# placed into `vendor/cryptopp` and `vendor/spdlog`.
RUN cd /app/vendor/cryptopp \
    && make -j"$(nproc)" static \
    && test -f libcryptopp.a

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
    && cmake -S server -B build-server -DROOT_DIR=/app -DCMAKE_BUILD_TYPE=Release -DCMAKE_RUNTIME_OUTPUT_DIRECTORY=/app/bin \
    && cmake --build build-server -j"$(nproc)"


FROM ubuntu:22.04 AS runtime

RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Runtime folders used by the binaries.
RUN mkdir -p logs versions

COPY --from=builder /app/bin/calliforniaServer /app/calliforniaServer
RUN chmod +x /app/calliforniaServer

EXPOSE 8081/tcp 8081/udp

ENTRYPOINT ["/app/calliforniaServer"]

