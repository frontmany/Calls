#pragma once
#include <functional>
#include <memory>
#include <queue>
#include <string>
#include <cstdint>
#include <vector>
#include <unordered_map>

#include "packetTypes.h"
#include "logger.h"
#include "chunk.h"

#include "asio.hpp"
#include "asio/ts/buffer.hpp"
#include "asio/ts/internet.hpp"

namespace calls {

class NetworkController {
private:
    struct ScreenFrame {
        std::vector<std::vector<unsigned char>> chunks;
        uint32_t id = 0;
        size_t totalSize = 0;
        uint16_t totalChunks = 0;
        uint16_t receivedChunks = 0;
    };

public:
    NetworkController();
    ~NetworkController();

    bool init(const std::string& host,
        const std::string& port,
        std::function<void(const unsigned char*, int, PacketType type)> onReceiveCallback,
        std::function<void()> onErrorCallback);

    void run();
    void stop();
    bool isRunning() const;
    void send(const std::vector<unsigned char>& data, PacketType type);
    void send(std::vector<unsigned char>&& data, PacketType type);
    void send(PacketType type);

private:
    void startReceive();
    void handleReceive(std::size_t bytes_transferred);
    void handleChunk(const unsigned char* data, std::size_t length, PacketType type);
    uint32_t generateId();

private:
    asio::io_context m_context;
    asio::ip::udp::socket m_socket;
    asio::ip::udp::endpoint m_serverEndpoint;
    asio::ip::udp::endpoint m_receivedFromEndpoint;

    std::vector<unsigned char> m_receiveBuffer;
    asio::executor_work_guard<asio::io_context::executor_type> m_workGuard;
    std::thread m_asioThread;

    const size_t m_maxPacketSize = 508;
    ScreenFrame m_pendingPacket;

    std::function<void(const unsigned char*, int, PacketType type)> m_onReceiveCallback;
    std::function<void()> m_onErrorCallback;
};

}