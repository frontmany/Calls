#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "network/UdpClient/packetReceiver.h"
#include "network/UdpClient/packetSender.h"
#include "asio.hpp"

namespace core::network::udp {

class Client {
public:
    Client();
    ~Client();

    bool initialize(const std::string& host,
        const std::string& port,
        std::function<void(const unsigned char*, int, uint32_t)> onReceive,
        std::function<void()> onConnectionDown,
        std::function<void()> onConnectionRestored);

    void start();
    void stop();
    bool isRunning() const;
    uint16_t getLocalMediaPort() const;

    bool send(const std::vector<unsigned char>& data, uint32_t type);
    bool send(std::vector<unsigned char>&& data, uint32_t type);
    void notifyConnectionRestored();
    void notifyConnectionDown();

private:
    uint64_t generateId();

    asio::io_context m_context;
    asio::ip::udp::socket m_socket;
    asio::ip::udp::endpoint m_serverEndpoint;
    asio::executor_work_guard<asio::io_context::executor_type> m_workGuard;
    std::thread m_asioThread;

    PacketReceiver m_packetReceiver;
    PacketSender m_packetSender;

    std::atomic<bool> m_running;
    std::atomic<uint64_t> m_nextPacketId;
    uint16_t m_localMediaPort{0};

    std::function<void(const unsigned char*, int, uint32_t)> m_onReceive;
    std::function<void()> m_onConnectionDown;
    std::function<void()> m_onConnectionRestored;
    std::atomic_bool m_connectionDownNotified{false};
};

}
