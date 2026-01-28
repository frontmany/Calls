#include "network/udp/mediaController.h"
#include "utilities/logger.h"

#include <thread>
#include <utility>

namespace core::network::udp {

MediaController::MediaController()
    : m_socket(m_context)
    , m_workGuard(asio::make_work_guard(m_context))
    , m_running(false)
    , m_nextPacketId(0U)
{
}

MediaController::~MediaController() {
    stop();
}

bool MediaController::initialize(const std::string& host,
    const std::string& port,
    std::function<void(const unsigned char*, int, uint32_t)> onReceive,
    std::function<void()> onConnectionDown,
    std::function<void()> onConnectionRestored)
{
    m_onReceive = std::move(onReceive);
    m_onConnectionDown = std::move(onConnectionDown);
    m_onConnectionRestored = std::move(onConnectionRestored);

    try {
        std::error_code errorCode;
        asio::ip::udp::resolver resolver(m_context);
        asio::ip::udp::resolver::results_type endpoints = resolver.resolve(asio::ip::udp::v4(), host, port, errorCode);

        if (errorCode) {
            LOG_ERROR("Media failed to resolve {}:{} - {}", host, port, errorCode.message());
            return false;
        }
        if (endpoints.empty()) {
            LOG_ERROR("Media no endpoints found for {}:{}", host, port);
            return false;
        }

        m_serverEndpoint = *endpoints.begin();

        if (m_socket.is_open()) {
            m_socket.close(errorCode);
            errorCode.clear();
        }

        m_socket.open(asio::ip::udp::v4(), errorCode);
        if (errorCode) {
            LOG_ERROR("Media failed to open socket: {}", errorCode.message());
            return false;
        }

        m_socket.set_option(asio::socket_base::reuse_address(true), errorCode);
        if (errorCode) {
            LOG_ERROR("Media failed to set reuse_address on socket: {}", errorCode.message());
            return false;
        }

        m_socket.bind(asio::ip::udp::endpoint(asio::ip::udp::v4(), 0), errorCode);
        if (errorCode) {
            LOG_ERROR("Media failed to bind socket: {}", errorCode.message());
            return false;
        }

        errorCode.clear();
        auto localEndpoint = m_socket.local_endpoint(errorCode);
        if (!errorCode)
            m_localMediaPort = static_cast<uint16_t>(localEndpoint.port());

        std::function<void()> errorHandler = []() {};
        auto pingReceivedHandler = [](uint32_t) {};

        if (!m_packetReceiver.initialize(m_socket, m_onReceive, errorHandler, pingReceivedHandler, m_serverEndpoint)) {
            LOG_ERROR("Media failed to initialize packet receiver");
            return false;
        }

        m_packetSender.initialize(m_socket, m_serverEndpoint, errorHandler);

        LOG_INFO("Media controller initialized, server: {}:{}, local port {}", host, port, m_localMediaPort);
        return true;
    }
    catch (const std::exception& exception) {
        LOG_ERROR("Media initialization error: {}", exception.what());
        stop();
        return false;
    }
}

void MediaController::start() {
    if (m_running.exchange(true))
        return;
    m_asioThread = std::thread([this]() { m_context.run(); });
    m_packetReceiver.start();
}

void MediaController::stop() {
    bool wasRunning = m_running.exchange(false);
    if (!wasRunning && !m_asioThread.joinable())
        return;

    m_packetSender.stop();
    m_packetReceiver.stop();

    std::error_code errorCode;
    if (m_socket.is_open()) {
        m_socket.cancel(errorCode);
        if (errorCode)
            LOG_WARN("Media failed to cancel socket operations: {}", errorCode.message());
        m_socket.close(errorCode);
        if (errorCode)
            LOG_WARN("Media failed to close socket: {}", errorCode.message());
    }

    m_workGuard.reset();
    m_context.stop();

    if (m_asioThread.joinable())
        m_asioThread.join();

    m_context.restart();
}

bool MediaController::isRunning() const {
    return m_running.load();
}

uint16_t MediaController::getLocalMediaPort() const {
    return m_localMediaPort;
}

bool MediaController::send(const std::vector<unsigned char>& data, uint32_t type) {
    if (type == 0 || type == 1) {
        LOG_ERROR("Media packet types 0 and 1 are reserved");
        return false;
    }
    Packet packet;
    packet.id = generateId();
    packet.type = type;
    packet.data = data;
    m_packetSender.send(packet);
    return true;
}

bool MediaController::send(std::vector<unsigned char>&& data, uint32_t type) {
    if (type == 0 || type == 1) {
        LOG_ERROR("Media packet types 0 and 1 are reserved");
        return false;
    }
    Packet packet;
    packet.id = generateId();
    packet.type = type;
    packet.data = std::move(data);
    m_packetSender.send(packet);
    return true;
}

uint64_t MediaController::generateId() {
    return m_nextPacketId.fetch_add(1U, std::memory_order_relaxed);
}

void MediaController::notifyConnectionRestored() {
    m_connectionDownNotified = false;
    m_packetReceiver.setConnectionDown(false);
}

void MediaController::notifyConnectionDown() {
    if (!m_connectionDownNotified.exchange(true)) {
        m_packetReceiver.setConnectionDown(true);
        if (m_onConnectionDown)
            m_onConnectionDown();
    }
}

}
