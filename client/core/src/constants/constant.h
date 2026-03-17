#pragma once

#include <cstddef>

namespace core::constant {
    constexpr std::size_t MAX_TCP_PACKET_BODY_SIZE_BYTES = 2 * 1024 * 1024; // 2 MB
    constexpr int SOCKET_CLOSE_CALLBACK_TIMEOUT_MS = 500;
    constexpr int LOGOUT_FLUSH_TIMEOUT_MS = 200;
}
