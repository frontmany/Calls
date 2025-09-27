#pragma once
#include <functional>
#include <memory>
#include <string>

#include "packetTypes.h"
#include "logger.h"

#include "asio.hpp"
#include "asio/ts/buffer.hpp"
#include "asio/ts/internet.hpp"

namespace calls {

class NetworkController {
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
    void send(std::vector<unsigned char>&& data, PacketType type);
    void send(std::string&& data, PacketType type);
    void send(PacketType type);

private:
    void startReceive();
    void handleReceive(std::size_t bytes_transferred);

    asio::io_context m_context;
    asio::ip::udp::socket m_socket;
    asio::ip::udp::endpoint m_serverEndpoint;
    asio::ip::udp::endpoint m_receivedFromEndpoint;

    std::vector<unsigned char> m_receiveBuffer;
    asio::executor_work_guard<asio::io_context::executor_type> m_workGuard;
    std::thread m_asioThread;

    std::function<void(const unsigned char*, int, PacketType type)> m_onReceiveCallback;
    std::function<void()> m_onErrorCallback;
    const size_t m_maxUdpPacketSize = 65507;
};

}