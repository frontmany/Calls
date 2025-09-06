#include "networkManager.h"
#include <iostream>
#include <thread>

NetworkManager::NetworkManager(const std::string& host, const std::string& port, std::function<void(const unsigned char*, int)> onReceiveCallback)
    : m_socket(m_context), m_workGuard(asio::make_work_guard(m_context))
{
    asio::ip::udp::resolver resolver(m_context);
    asio::ip::udp::resolver::results_type endpoints = resolver.resolve(asio::ip::udp::v4(), host, port);

    if (!endpoints.empty()) {
        m_serverEndpoint = *endpoints.begin();
    }
}

NetworkManager::~NetworkManager() {
    disconnect();
}

bool NetworkManager::connect() {
    try {
        m_socket.open(asio::ip::udp::v4());
        m_socket.bind(asio::ip::udp::endpoint(asio::ip::udp::v4(), 0));

        m_asioThread = std::thread([this]() {m_context.run(); });

        startReceive();
        m_isConnected = true;
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Connection error: " << e.what() << std::endl;
        return false;
    }
}

void NetworkManager::disconnect() {
    m_isConnected = false;


    if (m_socket.is_open()) {
        asio::error_code ec;
        m_socket.close(ec);
    }

    m_context.stop();

    if (m_asioThread.joinable()) {
        m_asioThread.join();
    }
}

bool NetworkManager::isConnected() const {
    return m_isConnected;
}

void NetworkManager::send(const unsigned char* data, int length) {
    if (!m_isConnected || !m_socket.is_open()) {
        throw std::runtime_error("Socket not connected");
    }

    m_socket.async_send_to(asio::buffer(data, length), m_serverEndpoint,
        [](const asio::error_code& error, std::size_t bytesSent) {
            if (error) {
                std::cerr << "Send error: " << error.message() << std::endl;
            }
        }
    );
}

void NetworkManager::startReceive() {
    m_socket.async_receive_from(asio::buffer(m_receiveBuffer), m_senderEndpoint,
        [this](const asio::error_code& error, std::size_t bytesTransferred) {
            handleReceive(error, bytesTransferred);
        }
    );
}

void NetworkManager::handleReceive(const asio::error_code& error, std::size_t bytesTransferred) {
    if (!error && bytesTransferred > 0) {
        if (m_onReceiveCallback) {
            m_onReceiveCallback(m_receiveBuffer.data(), static_cast<int>(bytesTransferred));
        }
    }
    else if (error != asio::error::operation_aborted) {
        std::cerr << "Receive error: " << error.message() << std::endl;
    }

    if (m_isConnected) {
        startReceive();
    }
}