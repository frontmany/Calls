#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "packetTypes.h"
#include "user.h"

#include "asio.hpp"
#include "asio/ts/buffer.hpp"
#include "asio/ts/internet.hpp"

class NetworkController {
public:
    NetworkController(const std::string& ip, const std::string& port,
        std::function<void(const unsigned char*, int, PacketType, const asio::ip::udp::endpoint&)> onReceiveCallback,
        std::function<void()> onNetworkErrorCallback);
    ~NetworkController();

    bool start();
    void stop();
    bool isRunning() const;
    void sendVoiceToClient(const asio::ip::udp::endpoint& clientEndpoint, const unsigned char* data, int length);
    void sendToClient(const asio::ip::udp::endpoint& clientEndpoint, const std::string& data, PacketType type);
    void sendToClient(const asio::ip::udp::endpoint& clientEndpoint, PacketType type);
   
private:
    void startReceive();
    void handleReceive(const asio::error_code& error, std::size_t bytesTransferred);

    asio::io_context m_context;
    asio::ip::udp::socket m_socket;
    asio::ip::udp::endpoint m_serverEndpoint;
    asio::ip::udp::endpoint m_receivedFromEndpoint;

    std::array<unsigned char, 64000> m_receiveBuffer{};
    std::function<void(const unsigned char*, int, PacketType, const asio::ip::udp::endpoint&)> m_onReceiveCallback;
    std::function<void()> m_onNetworkErrorCallback;

    bool m_isRunning = false;
    asio::executor_work_guard<asio::io_context::executor_type> m_workGuard;
    std::thread m_asioThread;
};
