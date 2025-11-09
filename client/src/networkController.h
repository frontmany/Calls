#pragma once
#include <functional>
#include <memory>
#include <string>
#include <deque>
#include <cstdint>
#include <vector>
#include <unordered_map>

#include "packetTypes.h"
#include "logger.h"
#include "screenPacket.h"

#include "asio.hpp"
#include "asio/ts/buffer.hpp"
#include "asio/ts/internet.hpp"

namespace calls {

class NetworkController {
private:
    struct ScreenFrameAssembly {
        std::vector<std::vector<unsigned char>> chunks;
        std::vector<bool> received;
        uint16_t totalChunks = 0;
        uint16_t receivedChunks = 0;
        std::size_t totalSize = 0;
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
    bool stopped() const;
    void sendScreen(std::vector<unsigned char>&& data, PacketType type);
    void sendVoice(std::vector<unsigned char>&& data, PacketType type);
    void sendPacket(std::string&& data, PacketType type);
    void sendPacket(PacketType type);

private:
    void startReceive();
    void handleReceive(std::size_t bytes_transferred);
    void handleScreenChunk(const unsigned char* data, std::size_t length);
    void enqueueScreenFrame(std::vector<unsigned char>&& data, PacketType type, uint32_t frameId);
    void scheduleNextScreenDatagram();

    asio::io_context m_context;
    asio::ip::udp::socket m_socket;
    asio::ip::udp::endpoint m_serverEndpoint;
    asio::ip::udp::endpoint m_receivedFromEndpoint;

    std::vector<unsigned char> m_receiveBuffer;
    asio::executor_work_guard<asio::io_context::executor_type> m_workGuard;
    std::thread m_asioThread;

    const size_t m_maxUdpPacketSize = 65507;
    std::deque<std::shared_ptr<ScreenDatagram>> m_screenDatagramQueue;
    bool m_screenSendInProgress = false;
    uint32_t m_nextScreenFrameId = 0;
    std::vector<unsigned char> m_reassembledScreenFrame;
    std::unordered_map<uint32_t, ScreenFrameAssembly> m_pendingScreenFrames;

    std::function<void(const unsigned char*, int, PacketType type)> m_onReceiveCallback;
    std::function<void()> m_onErrorCallback;
};

}