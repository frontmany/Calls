#pragma once

#include <cstddef>

namespace server::constant {
    constexpr std::size_t MAX_TCP_PACKET_BODY_SIZE_BYTES = 2 * 1024 * 1024; // 2 MB

#ifdef _WIN32
    // Windows socket error codes used for silent network-shutdown handling.
    constexpr int WSA_ERROR_NOT_SOCKET = 10038;        // WSAENOTSOCK
    constexpr int WSA_ERROR_NOT_CONNECTED = 10057;     // WSAENOTCONN
    constexpr int WSA_ERROR_CONN_RESET = 10054;        // WSAECONNRESET
    constexpr int WSA_ERROR_SHUTDOWN = 10058;          // WSAESHUTDOWN
    constexpr int WSA_ERROR_CONN_ABORTED = 10053;      // WSAECONNABORTED
    constexpr int WSA_ERROR_INTERRUPTED = 10004;       // WSAEINTR
#endif
}
