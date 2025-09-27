#include "networkController.h"
#include <thread>
#include <cstring>
#include <iostream>

using namespace calls;

NetworkController::NetworkController()
    : m_socket(m_context),
    m_workGuard(asio::make_work_guard(m_context))
{
}

NetworkController::~NetworkController() {
    stop();
}

bool NetworkController::init(const std::string& host,
    const std::string& port,
    std::function<void(const unsigned char*, int, PacketType type)> onReceiveCallback,
    std::function<void()> onErrorCallback)
{
    m_receiveBuffer.resize(m_maxUdpPacketSize);
    m_onErrorCallback = onErrorCallback;
    m_onReceiveCallback = onReceiveCallback;

    try {
        asio::ip::udp::resolver resolver(m_context);
        asio::ip::udp::resolver::results_type endpoints = resolver.resolve(asio::ip::udp::v4(), host, port);

        if (endpoints.empty()) {
            DEBUG_LOG("No endpoints found for " + host + ":" + port);
            return false;
        }

        m_serverEndpoint = *endpoints.begin();
        m_socket.open(asio::ip::udp::v4());
        m_socket.bind(asio::ip::udp::endpoint(asio::ip::udp::v4(), 0));

        return true;
    }
    catch (const std::exception& e) {
        DEBUG_LOG(std::string("Initialization error: ") + e.what());
        m_workGuard.reset();
        stop();
        return false;
    }
}

void NetworkController::run() {
    m_asioThread = std::thread([this]() {m_context.run(); });
    startReceive();
}

void NetworkController::stop() {
    if (m_socket.is_open()) {
        asio::error_code ec;
        m_socket.close(ec);
        if (ec) {
            DEBUG_LOG("Socket closing error: " + ec.message());
        }
    }

    if (!m_context.stopped()) {
        m_context.stop();
    }

    if (m_asioThread.joinable()) {
        m_asioThread.join();
    }
}

bool NetworkController::stopped() const {
    return m_context.stopped();
}

void NetworkController::send(std::vector<unsigned char>&& data, PacketType type) {
    asio::post(m_socket.get_executor(),
        [this, data = std::move(data), type, endpoint = m_serverEndpoint]() mutable {
            if (!m_socket.is_open()) {
                return;
            }

            const size_t totalSize = data.size() + sizeof(PacketType);
            if (totalSize > m_maxUdpPacketSize) {
                DEBUG_LOG("Voice Packet too large: " + std::to_string(totalSize) + " bytes");
                return;
            }

            std::array<asio::const_buffer, 2> buffers = {
                asio::buffer(data.data(), data.size()),
                asio::buffer(&type, sizeof(PacketType))
            };

            m_socket.async_send_to(buffers, endpoint,
                [this](const asio::error_code& error, std::size_t bytesSent) {
                    if (error && error != asio::error::operation_aborted) {
                        DEBUG_LOG("Send error: " + error.message());
                        m_onErrorCallback();
                    }
                }
            );
        }
    );
}

void NetworkController::send(std::string&& data, PacketType type) {
    asio::post(m_socket.get_executor(),
        [this, data = std::move(data), type, endpoint = m_serverEndpoint]() mutable {
            if (!m_socket.is_open()) {
                return;
            }

            const size_t totalSize = data.size() + sizeof(PacketType);
            if (totalSize > m_maxUdpPacketSize) {
                DEBUG_LOG("Packet too large: " + std::to_string(totalSize) + " bytes");
                return;
            }

            std::array<asio::const_buffer, 2> buffers = {
                asio::buffer(data.data(), data.size()),
                asio::buffer(&type, sizeof(PacketType))
            };

            m_socket.async_send_to(buffers, endpoint,
                [this](const asio::error_code& error, std::size_t bytesSent) {
                    if (error && error != asio::error::operation_aborted) {
                        DEBUG_LOG("Send error: " + error.message());
                        m_onErrorCallback();
                    }
                }
            );
        }
    );
}

void NetworkController::send(PacketType type) {
    asio::post(m_socket.get_executor(),
        [this, type]() {
            if (!m_socket.is_open()) {
                return;
            }

            std::array<asio::const_buffer, 1> buffer = {
                asio::buffer(&type, sizeof(PacketType))
            };

            m_socket.async_send_to(buffer, m_serverEndpoint,
                [this](const asio::error_code& error, std::size_t bytesSent) {
                    if (error && error != asio::error::operation_aborted) {
                        DEBUG_LOG("Send error: " + error.message());
                        m_onErrorCallback();
                    }
                }
            );
        }
    );
}

void NetworkController::startReceive() {
    if (!m_socket.is_open()) return;

    m_socket.async_receive_from(asio::buffer(m_receiveBuffer), m_receivedFromEndpoint,
        [this](const asio::error_code& ec, std::size_t bytesTransferred) {
            if (ec && ec != asio::error::operation_aborted) {
                m_onErrorCallback();
            }

            handleReceive(bytesTransferred);
        }
    );
}

void NetworkController::handleReceive(std::size_t bytesTransferred) {
    if (bytesTransferred < sizeof(PacketType)) {
        DEBUG_LOG("Received packet too small: " + std::to_string(bytesTransferred) + " bytes");
        startReceive();
        return;
    }

    PacketType receivedType;
    std::memcpy(&receivedType,
        m_receiveBuffer.data() + bytesTransferred - sizeof(PacketType),
        sizeof(PacketType));

    const int dataLength = static_cast<int>(bytesTransferred - sizeof(PacketType));
    m_onReceiveCallback(m_receiveBuffer.data(), dataLength, receivedType);

    startReceive();
}