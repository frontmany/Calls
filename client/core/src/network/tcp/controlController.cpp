#include "network/tcp/controlController.h"
#include "network/tcp/packetsReceiver.h"
#include "network/tcp/packetsSender.h"
#include "utilities/logger.h"
#include "utilities/crypto.h"
#include "utilities/errorCodeForLog.h"

#include <array>
#include <chrono>
#include <cstdint>
#include <thread>

#if defined(_MSC_VER)
#include <cstdlib>
#endif

namespace core::network::tcp {

using namespace core::utilities::crypto;

ControlController::ControlController(OnControlPacket onControlPacket, std::function<void()> onConnectionDown)
    : m_socket(m_context)
    , m_onControlPacket(std::move(onControlPacket))
    , m_onConnectionDown(std::move(onConnectionDown))
{
}

ControlController::~ControlController() {
    disconnect();
}

void ControlController::connect(const std::string& host, const std::string& port) {
    if (m_connecting.exchange(true)) {
        LOG_WARN("Control already connecting/connected");
        return;
    }
    m_shuttingDown = false;

    uint16_t portNumber = 0;
    try {
        portNumber = static_cast<uint16_t>(std::stoul(port));
    }
    catch (...) {
        LOG_ERROR("Control invalid port: {}", port);
        m_connecting = false;
        if (m_onConnectionDown)
            m_onConnectionDown();
        return;
    }

    if (m_context.stopped()) {
        m_context.restart();
        m_socket = asio::ip::tcp::socket(m_context);
    }

    m_workGuard = std::make_unique<asio::executor_work_guard<asio::io_context::executor_type>>(
        asio::make_work_guard(m_context));

    if (!m_asioThread.joinable())
        m_asioThread = std::thread([this]() { m_context.run(); });

    runConnect(host, portNumber);
}

void ControlController::runConnect(const std::string& host, uint16_t portNumber) {
    auto onConnected = [this](std::error_code errorCode, const asio::ip::tcp::endpoint&) {
        if (errorCode) {
            if (errorCode != asio::error::operation_aborted)
                LOG_ERROR("Control connect failed: {}", core::utilities::errorCodeForLog(errorCode));
            m_connecting = false;
            if (m_onConnectionDown) m_onConnectionDown();
            return;
        }
        std::error_code optionError;
        m_socket.set_option(asio::ip::tcp::socket::keep_alive(true), optionError);
        if (optionError)
            LOG_WARN("Control keepalive failed: {}", core::utilities::errorCodeForLog(optionError));
        readHandshake();
    };

    std::error_code resolveError;
    asio::ip::address address = asio::ip::make_address(host, resolveError);
    if (!resolveError) {
        std::array<asio::ip::tcp::endpoint, 1> endpoints = { asio::ip::tcp::endpoint(address, portNumber) };
        asio::async_connect(m_socket, endpoints, std::move(onConnected));
        return;
    }

    asio::ip::tcp::resolver resolver(m_context);
    resolver.async_resolve(host, std::to_string(portNumber),
        [this, host, portNumber, onConnected = std::move(onConnected)](
            std::error_code resolveError, asio::ip::tcp::resolver::results_type results) mutable {
            if (resolveError) {
                LOG_ERROR("Control resolve {}:{} failed: {}", host, portNumber, core::utilities::errorCodeForLog(resolveError));
                m_connecting = false;
                if (m_onConnectionDown) m_onConnectionDown();
                return;
            }
            asio::async_connect(m_socket, results, std::move(onConnected));
        });
}

void ControlController::readHandshake() {
    asio::async_read(m_socket, asio::buffer(&m_handshakeIn, sizeof(uint64_t)),
        [this](std::error_code errorCode, std::size_t) {
            if (errorCode) {
                if (errorCode != asio::error::operation_aborted)
                    LOG_ERROR("Control handshake read error: {}", core::utilities::errorCodeForLog(errorCode));
                m_connecting = false;
                if (m_onConnectionDown) m_onConnectionDown();
                return;
            }

            m_handshakeOut = scramble(m_handshakeIn);
            writeHandshake();
        });
}

void ControlController::writeHandshake() {
    asio::async_write(m_socket, asio::buffer(&m_handshakeOut, sizeof(uint64_t)),
        [this](std::error_code errorCode, std::size_t) {
            if (errorCode) {
                if (errorCode != asio::error::operation_aborted)
                    LOG_ERROR("Control handshake write error: {}", core::utilities::errorCodeForLog(errorCode));
                m_connecting = false;
                if (m_onConnectionDown) m_onConnectionDown();
                return;
            }
            readHandshakeConfirmation();
        });
}

void ControlController::readHandshakeConfirmation() {
    asio::async_read(m_socket, asio::buffer(&m_handshakeConfirmation, sizeof(uint64_t)),
        [this](std::error_code errorCode, std::size_t) {
            if (errorCode) {
                if (errorCode != asio::error::operation_aborted)
                    LOG_ERROR("Control handshake confirmation read error: {}", core::utilities::errorCodeForLog(errorCode));
                m_connecting = false;
                if (m_onConnectionDown) m_onConnectionDown();
                return;
            }

            if (m_handshakeConfirmation != m_handshakeOut) {
                LOG_WARN("Control handshake validation failed");
                m_connecting = false;
                if (m_onConnectionDown) m_onConnectionDown();
                return;
            }
            m_connecting = false;
            m_connected = true;
            initializeAfterHandshake();
        });
}

void ControlController::initializeAfterHandshake() {
    m_receiver = std::make_unique<PacketsReceiver>(m_socket,
        [this](Packet&& packet) { m_inQueue.push(std::move(packet)); },
        [this]() {
            if (!m_shuttingDown && m_connected.exchange(false)) {
                LOG_WARN("Control disconnected (receive)");
                if (m_onConnectionDown) m_onConnectionDown();
            }
        });

    m_sender = std::make_unique<PacketsSender>(m_socket, m_outQueue,
        [this]() {
            if (!m_shuttingDown && m_connected.exchange(false)) {
                LOG_WARN("Control disconnected (send)");
                if (m_onConnectionDown) m_onConnectionDown();
            }
        });

    m_processThread = std::thread([this]() { processPacketQueue(); });

    m_receiver->startReceiving();

    LOG_INFO("Control channel connected");
}

void ControlController::processPacketQueue() {
    using namespace std::chrono_literals;
    while (!m_shuttingDown) {
        auto optionalPacket = m_inQueue.pop_for(500ms);
        if (!optionalPacket)
            continue;
        Packet packet = std::move(*optionalPacket);
        const unsigned char* data = packet.body.empty() ? nullptr : packet.body.data();
        size_t size = packet.body.size();
        if (m_onControlPacket)
            m_onControlPacket(packet.type, data, size);
    }
}

void ControlController::disconnect() {
    m_shuttingDown = true;
    m_connected = false;
    m_connecting = false;

    if (m_processThread.joinable()) {
        m_processThread.join();
    }

    asio::post(m_context, [this]() {
        if (m_socket.is_open()) {
            std::error_code errorCode;
            m_socket.close(errorCode);
            if (errorCode)
                LOG_ERROR("Control socket close error: {}", core::utilities::errorCodeForLog(errorCode));
        }
    });

    m_workGuard.reset();
    if (m_asioThread.joinable()) {
        m_asioThread.join();
    }

    m_receiver.reset();
    m_sender.reset();
    m_outQueue.clear();
    m_shuttingDown = false;
}

bool ControlController::isConnected() const {
    return m_connected.load() && m_socket.is_open() && !m_shuttingDown;
}

bool ControlController::send(uint32_t type, const std::vector<unsigned char>& body) {
    return send(type, body.data(), body.size());
}

bool ControlController::send(uint32_t type, const void* data, size_t size) {
    if (!m_sender || !isConnected())
        return false;
    Packet packet;
    packet.type = type;
    packet.bodySize = static_cast<uint32_t>(size);
    if (size > 0 && data) {
        const auto* bytes = static_cast<const uint8_t*>(data);
        packet.body.assign(bytes, bytes + size);
    }
    bool wasEmpty = m_outQueue.empty();
    m_outQueue.push(std::move(packet));
    if (wasEmpty)
        m_sender->send();
    return true;
}

}
