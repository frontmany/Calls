#pragma once

#include <functional>
#include <memory>
#include <string>

#include "asio.hpp"
#include "asio/ts/buffer.hpp"
#include "asio/ts/internet.hpp"

class NetworkManager {
public:
    NetworkManager(const std::string& host, const std::string& port, std::function<void(const unsigned char*, int)> onReceiveCallback);
    ~NetworkManager();

    bool connect();
    void disconnect();
    bool isConnected() const;
    void send(const unsigned char* data, int length);

private:
    void startReceive();
    void handleReceive(const asio::error_code& error, std::size_t bytes_transferred);

    asio::io_context m_context;
    asio::ip::udp::socket m_socket;
    asio::ip::udp::endpoint m_serverEndpoint;
    asio::ip::udp::endpoint m_senderEndpoint;

    std::array<unsigned char, 1024> m_receiveBuffer;
    std::function<void(const unsigned char*, int)> m_onReceiveCallback;
    bool m_isConnected;

    std::thread m_contextThread;
    asio::executor_work_guard<asio::io_context::executor_type, void, void> m_workGuard;
    std::thread m_asioThread;
};