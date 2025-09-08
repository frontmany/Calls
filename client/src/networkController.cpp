#include "networkController.h"
#include <iostream>
#include <thread>
#include <cstring>

NetworkController::NetworkController(const std::string& host,
    const std::string& port,
    std::function<void(const unsigned char*, int, PacketType type)> onReceiveCallback,
    std::function<void()> onReceiveErrorCallback)
    : m_socket(m_context),
    m_workGuard(asio::make_work_guard(m_context)),
    m_onReceiveCallback(onReceiveCallback),
    m_onNetworkErrorCallback(onReceiveErrorCallback)
{
    asio::ip::udp::resolver resolver(m_context);
    asio::ip::udp::resolver::results_type endpoints = resolver.resolve(asio::ip::udp::v4(), host, port);

    if (endpoints.empty()) {
        throw std::runtime_error("No endpoints found for " + host + ":" + port);
    }

    m_serverEndpoint = *endpoints.begin();
}

NetworkController::~NetworkController() {
    disconnect();
}

bool NetworkController::connect() {
    try {
        m_socket.open(asio::ip::udp::v4());
        m_socket.bind(asio::ip::udp::endpoint(asio::ip::udp::v4(), 0));

        m_asioThread = std::thread([this]() { m_context.run(); });
        m_isConnected = true;
        startReceive();

        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Connection error: " << e.what() << std::endl;
        m_isConnected = false;
        return false;
    }
}

void NetworkController::disconnect() {
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

bool NetworkController::isConnected() const {
    return m_isConnected;
}

bool NetworkController::send(const unsigned char* data, int length, PacketType type) {
    if (!m_isConnected || !m_socket.is_open()) {
        return false;
    }

    const size_t totalSize = length + sizeof(PacketType);
    if (totalSize > m_receiveBuffer.size()) {
        std::cerr << "Voice Packet too large: " << totalSize << " bytes" << std::endl;
        return false;
    }

    std::array<asio::const_buffer, 2> buffers = {
        asio::buffer(data, length),
        asio::buffer(&type, sizeof(PacketType))
    };

    m_socket.async_send_to(buffers, m_serverEndpoint,
        [](const asio::error_code& error, std::size_t bytesSent) {
            if (error) {
                std::cerr << "Send error: " << error.message() << std::endl;
            }
        }
    );

    return true;
}

bool NetworkController::send(const std::string& data, PacketType type) {
    if (!m_isConnected || !m_socket.is_open()) {
        return false;
    }

    const size_t totalSize = data.size() + sizeof(PacketType);
    if (totalSize > m_receiveBuffer.size()) {
        std::cerr << "Packet too large: " << totalSize << " bytes" << std::endl;
        return false;
    }

    std::array<asio::const_buffer, 2> buffers = {
        asio::buffer(reinterpret_cast<const unsigned char*>(data.data()), data.size()),
        asio::buffer(&type, sizeof(PacketType))
    };

    m_socket.async_send_to(buffers, m_serverEndpoint,
        [this](const asio::error_code& error, std::size_t bytesSent) {
            if (error && error != asio::error::operation_aborted) {
                std::cerr << "Send error: " << error.message() << std::endl;
                m_onNetworkErrorCallback();
            }
        }
    );

    return true;
}

bool NetworkController::send(PacketType type) {
    if (!m_isConnected || !m_socket.is_open()) {
        return false;
    }

    std::array<asio::const_buffer, 1> buffer = {
        asio::buffer(&type, sizeof(PacketType))
    };

    m_socket.async_send_to(buffer, m_serverEndpoint,
        [this](const asio::error_code& error, std::size_t bytesSent) {
            if (error && error != asio::error::operation_aborted) {
                std::cerr << "Send error: " << error.message() << std::endl;
                m_onNetworkErrorCallback();
            }
        }
    );

    return true;
}

void NetworkController::startReceive() {
    m_socket.async_receive_from(asio::buffer(m_receiveBuffer), m_receivedFromEndpoint,
        [this](const asio::error_code& ec, std::size_t bytesTransferred) {
            if (ec) {
                if (ec != asio::error::operation_aborted) {
                    std::cerr << "Receive error: " << ec.message() << std::endl;
                    m_onNetworkErrorCallback();
                }
                return;
            }

            handleReceive(bytesTransferred);
        }
    );
}

void NetworkController::handleReceive(std::size_t bytesTransferred) {
    if (bytesTransferred <= sizeof(PacketType)) {
        std::cerr << "Received packet too small: " << bytesTransferred << " bytes" << std::endl;

        if (m_isConnected) {
            startReceive();
        }
        return;
    }

    PacketType receivedType;
    std::memcpy(&receivedType,
        m_receiveBuffer.data() + bytesTransferred - sizeof(PacketType),
        sizeof(PacketType));

    const int data_length = static_cast<int>(bytesTransferred - sizeof(PacketType));

    if (m_onReceiveCallback) {
        m_onReceiveCallback(m_receiveBuffer.data(), data_length, receivedType);
    }

    if (m_isConnected) {
        startReceive();
    }
}