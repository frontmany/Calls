#include "networkController.h"
#include "logger.h"
#include <iostream>
#include <thread>
#include <cstring>
#include <sstream>

NetworkController::NetworkController(const std::string& port,
    std::function<void(const unsigned char*, int, PacketType, const asio::ip::udp::endpoint&)> onReceiveCallback,
    std::function<void()> onNetworkErrorCallback)
    : m_socket(m_context),
    m_workGuard(asio::make_work_guard(m_context)),
    m_onReceiveCallback(onReceiveCallback),
    m_onNetworkErrorCallback(onNetworkErrorCallback)
{
    asio::ip::udp::resolver resolver(m_context);
    asio::ip::udp::resolver::results_type endpoints = resolver.resolve(asio::ip::udp::v4(), "0.0.0.0", port);

    if (endpoints.empty()) {
        throw std::runtime_error("No endpoints found for port " + port);
    }

    m_serverEndpoint = *endpoints.begin();
}

NetworkController::~NetworkController() {
    stop();
}

bool NetworkController::start() {
    try {
        m_socket.open(asio::ip::udp::v4());
        m_socket.bind(m_serverEndpoint);

        m_asioThread = std::thread([this]() { m_context.run(); });
        m_isRunning = true;
        startReceive();

        LOG_INFO("UDP Server started on {}:{}", m_serverEndpoint.address().to_string(), m_serverEndpoint.port());
        return true;
    }
    catch (const std::exception& e) {
        LOG_ERROR("Server start error: {}", e.what());
        m_isRunning = false;
        return false;
    }
}

void NetworkController::stop() {
    m_isRunning = false;

    if (m_socket.is_open()) {
        asio::error_code ec;
        m_socket.close(ec);
    }

    m_context.stop();

    if (m_asioThread.joinable()) {
        m_asioThread.join();
    }
}

bool NetworkController::isRunning() const {
    return m_isRunning;
}

void NetworkController::sendVoiceToClient(const asio::ip::udp::endpoint& clientEndpoint,
    const unsigned char* data, int length) {
    auto sharedData = std::make_shared<std::vector<unsigned char>>(data, data + length);

    asio::post(m_socket.get_executor(),
        [this, clientEndpoint, sharedData]() {
            if (!m_isRunning || !m_socket.is_open()) {
                return;
            }

            const size_t totalSize = sharedData->size() + sizeof(PacketType);
            if (totalSize > m_receiveBuffer.size()) {
                LOG_WARN("Voice packet too large: {} bytes (max: {})", totalSize, m_receiveBuffer.size());
                return;
            }

            PacketType type = PacketType::VOICE;

            std::array<asio::const_buffer, 2> buffers = {
                asio::buffer(sharedData->data(), sharedData->size()),
                asio::buffer(&type, sizeof(PacketType))
            };

            m_socket.async_send_to(buffers, clientEndpoint,
                [](const asio::error_code& error, std::size_t bytesSent) {
                    if (error) {
                        LOG_ERROR("Voice packet send error: {}", error.message());
                    }
                }
            );
        }
    );
}

void NetworkController::sendToClient(const asio::ip::udp::endpoint& clientEndpoint, PacketType type) {
    asio::post(m_socket.get_executor(),
        [this, clientEndpoint, type]() {
            if (!m_isRunning || !m_socket.is_open()) {
                return;
            }

            std::array<asio::const_buffer, 1> buffer = {
                asio::buffer(&type, sizeof(PacketType))
            };

            m_socket.async_send_to(buffer, clientEndpoint,
                [this](const asio::error_code& error, std::size_t bytesSent) {
                    if (error && error != asio::error::operation_aborted) {
                        LOG_ERROR("Packet send error: {}", error.message());
                        m_onNetworkErrorCallback();
                    }
                }
            );
        }
    );
}

void NetworkController::sendToClient(const asio::ip::udp::endpoint& clientEndpoint,
    const std::string& data, PacketType type) {

    auto sharedData = std::make_shared<std::string>(data);

    asio::post(m_socket.get_executor(),
        [this, clientEndpoint, sharedData, type]() {
            if (!m_isRunning || !m_socket.is_open()) {
                return;
            }

            const size_t totalSize = sharedData->size() + sizeof(PacketType);
            if (totalSize > m_receiveBuffer.size()) {
                LOG_WARN("Data packet too large: {} bytes (max: {})", totalSize, m_receiveBuffer.size());
                return;
            }

            std::array<asio::const_buffer, 2> buffers = {
                asio::buffer(sharedData->data(), sharedData->size()),
                asio::buffer(&type, sizeof(PacketType))
            };

            m_socket.async_send_to(buffers, clientEndpoint,
                [sharedData](const asio::error_code& error, std::size_t bytesSent) {
                    if (error && error != asio::error::operation_aborted) {
                        LOG_ERROR("Data packet send error: {}", error.message());
                    }
                }
            );
        }
    );
}

void NetworkController::startReceive() {
    m_socket.async_receive_from(asio::buffer(m_receiveBuffer), m_receivedFromEndpoint,
        [this](const asio::error_code& ec, std::size_t bytesTransferred) {
            handleReceive(ec, bytesTransferred);
        }
    );
}

void NetworkController::handleReceive(const asio::error_code& error, std::size_t bytesTransferred) {
    if (error) {
        if (error != asio::error::operation_aborted) {
            if (error == asio::error::connection_refused) {
                startReceive();
                return;
            }
            else {
                m_onNetworkErrorCallback();
                return;
            }
        }
    }

    if (bytesTransferred < sizeof(PacketType)) {
        LOG_WARN("Received packet too small: {} bytes (minimum: {})", bytesTransferred, sizeof(PacketType));
        if (m_isRunning) {
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
        m_onReceiveCallback(m_receiveBuffer.data(), data_length, receivedType, m_receivedFromEndpoint);
    }

    if (m_isRunning) {
        startReceive();
    }

}