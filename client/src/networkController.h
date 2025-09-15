#pragma once
#include <functional>
#include <memory>
#include <string>

#include "packetTypes.h"

#include "asio.hpp"
#include "asio/ts/buffer.hpp"
#include "asio/ts/internet.hpp"


class NetworkController {
public:
    NetworkController(const std::string& host,
        const std::string& port,
        std::function<void(const unsigned char*, int, PacketType type)> onReceiveCallback,
        std::function<void()> onNetworkErrorCallback);
    ~NetworkController();

    bool connect();
    void disconnect();
    bool isConnected() const;
    void send(std::vector<unsigned char>&& data, PacketType type);
    void send(const std::string& data, PacketType type);
    void send(PacketType type);

private:
    void startReceive();
    void handleReceive(std::size_t bytes_transferred);

    asio::io_context m_context;
    asio::ip::udp::socket m_socket;
    asio::ip::udp::endpoint m_serverEndpoint;
    asio::ip::udp::endpoint m_receivedFromEndpoint;

    std::array<unsigned char, 64000> m_receiveBuffer{};
    std::function<void(const unsigned char*, int, PacketType type)> m_onReceiveCallback;
    std::function<void()> m_onNetworkErrorCallback;

    bool m_isConnected = false;
    asio::executor_work_guard<asio::io_context::executor_type> m_workGuard;
    std::thread m_asioThread;
};