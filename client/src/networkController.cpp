#include "networkController.h"
#include <thread>
#include <cstring>
#include <iostream>


#ifdef DEBUG
#define log(message) std::cout << message
#else
#define log(message) 
#endif

using namespace calls;

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
        log("No endpoints found for " + host + ":" + port);
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
        log("Connection error: " << e.what() << std::endl);
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

void NetworkController::send(std::vector<unsigned char>&& data, PacketType type) {
    auto shared_data = std::make_shared<std::vector<unsigned char>>(std::move(data));

    asio::post(m_socket.get_executor(),
        [this, shared_data, type]() {
            if (!m_isConnected || !m_socket.is_open()) {
                return;
            }

            const size_t totalSize = shared_data->size() + sizeof(PacketType);
            if (totalSize > m_receiveBuffer.size()) {
                log("Voice Packet too large: " << totalSize << " bytes" << std::endl);
                return;
            }

            std::array<asio::const_buffer, 2> buffers = {
                asio::buffer(shared_data->data(), shared_data->size()),
                asio::buffer(&type, sizeof(PacketType))
            };

            m_socket.async_send_to(buffers, m_serverEndpoint,
                [](const asio::error_code& error, std::size_t bytesSent) {
                    if (error) {
                        log("Send error: " << error.message() << std::endl);
                    }
                }
            );
        }
    );
}

void NetworkController::send(const std::string& data, PacketType type) {
    auto sharedData = std::make_shared<std::string>(data);

    asio::post(m_socket.get_executor(),
        [this, sharedData, type]() {
            if (!m_isConnected || !m_socket.is_open()) {
                return;
            }

            const size_t totalSize = sharedData->size() + sizeof(PacketType);
            if (totalSize > m_receiveBuffer.size()) {
                log("Packet too large" << std::endl);
                return;
            }

            std::array<asio::const_buffer, 2> buffers = {
                asio::buffer(sharedData->data(), sharedData->size()),
                asio::buffer(&type, sizeof(PacketType))
            };

            m_socket.async_send_to(buffers, m_serverEndpoint,
                [this](const asio::error_code& error, std::size_t bytesSent) {
                    if (error && error != asio::error::operation_aborted) {
                        log("Send error: " << error.message() << std::endl);
                        m_onNetworkErrorCallback();
                    }
                }
            );
        }
    );
}

void NetworkController::send(PacketType type) {
    asio::post(m_socket.get_executor(),
        [this, type]() {
            if (!m_isConnected || !m_socket.is_open()) {
                return;
            }

            std::array<asio::const_buffer, 1> buffer = {
                asio::buffer(&type, sizeof(PacketType))
            };

            m_socket.async_send_to(buffer, m_serverEndpoint,
                [this](const asio::error_code& error, std::size_t bytesSent) {
                    if (error && error != asio::error::operation_aborted) {
                        log("Send error: " << error.message() << std::endl);
                        m_onNetworkErrorCallback();
                    }
                }
            );
        }
    );
}

void NetworkController::startReceive() {
    m_socket.async_receive_from(asio::buffer(m_receiveBuffer), m_receivedFromEndpoint,
        [this](const asio::error_code& ec, std::size_t bytesTransferred) {
            if (ec) {
                if (ec != asio::error::operation_aborted) {
                    m_onNetworkErrorCallback();
                }
            }

            handleReceive(bytesTransferred);
        }
    );
}

void NetworkController::handleReceive(std::size_t bytesTransferred) {
    if (bytesTransferred < sizeof(PacketType)) {
        log("Received packet too small: " << bytesTransferred << " bytes" << std::endl);

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