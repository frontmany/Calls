#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <array>
#include <unordered_set>

#include "packetTypes.h"
#include "user.h"
#include "chunk.h"

#include "asio.hpp"
#include "asio/ts/buffer.hpp"
#include "asio/ts/internet.hpp"

class NetworkController {
public:
    static constexpr std::size_t RECEIVE_BUFFER_SIZE = 8192;

    NetworkController(const std::string& port,
        std::function<void(const unsigned char*, int, PacketType, const asio::ip::udp::endpoint&)> onReceiveCallback,
        std::function<void()> onNetworkErrorCallback);
    ~NetworkController();

    bool start();
    void stop();
    bool isRunning() const;
    void sendVoiceToClient(const asio::ip::udp::endpoint& clientEndpoint, const unsigned char* data, int length);
    void sendScreenToClient(const asio::ip::udp::endpoint& clientEndpoint, const unsigned char* data, int length);
    void sendToClient(const asio::ip::udp::endpoint& clientEndpoint, const std::string& data, PacketType type);
    void sendToClient(const asio::ip::udp::endpoint& clientEndpoint, PacketType type);
   
private:
    void forwardChunkToClient(const asio::ip::udp::endpoint& clientEndpoint,
        const unsigned char* data, std::size_t length, PacketType type);
    struct PendingPacket
    {
        std::vector<std::vector<unsigned char>> chunks;
        std::size_t totalSize = 0;
        uint16_t totalChunks = 0;
        uint16_t receivedChunks = 0;
    };

    struct PacketTypeHash
    {
        std::size_t operator()(PacketType type) const noexcept
        {
            return static_cast<std::size_t>(type);
        }
    };

    void startReceive();
    void handleReceive(const asio::error_code& error, std::size_t bytesTransferred);
    void sendDataToClient(const asio::ip::udp::endpoint& clientEndpoint, const unsigned char* data, std::size_t length, PacketType type); \
    std::string makeEndpointKey(const asio::ip::udp::endpoint& endpoint);
    uint32_t generateId();

    asio::io_context m_context;
    asio::ip::udp::socket m_socket;
    asio::ip::udp::endpoint m_serverEndpoint;
    asio::ip::udp::endpoint m_receivedFromEndpoint;

    std::array<unsigned char, RECEIVE_BUFFER_SIZE> m_receiveBuffer{};
    std::function<void(const unsigned char*, int, PacketType, const asio::ip::udp::endpoint&)> m_onReceiveCallback;
    std::function<void()> m_onNetworkErrorCallback;

    bool m_isRunning = false;
    asio::executor_work_guard<asio::io_context::executor_type> m_workGuard;
    std::thread m_asioThread;
    const std::size_t m_maxPacketSize = 508;
    std::unordered_set<PacketType, PacketTypeHash> m_forwardWithoutAssembly;
    std::unordered_map<std::string, std::unordered_map<uint32_t, PendingPacket>> m_pendingPackets;
};
