#include "client.h"
#include "network/tcp/packetReceiver.h"
#include "network/tcp/packetSender.h"
#include "utilities/logger.h"
#include "utilities/crypto.h"
#include "utilities/errorCodeForLog.h"

#include <array>
#include <chrono>
#include <cstdint>
#include <thread>

#ifdef _WIN32
#include <mstcpip.h>
#elif defined(__linux__) || defined(__APPLE__)
#include <netinet/tcp.h>
#endif

namespace core::network::tcp {

using namespace core::utilities::crypto;

static void configureKeepalive(asio::ip::tcp::socket& socket) {
    std::error_code ec;
    socket.set_option(asio::ip::tcp::socket::keep_alive(true), ec);
    if (ec) {
        LOG_WARN("Control failed to enable keepalive: {}", core::utilities::errorCodeForLog(ec));
        return;
    }

#ifdef _WIN32
    struct tcp_keepalive ka{};
    ka.onoff = 1;
    ka.keepalivetime = 10000;
    ka.keepaliveinterval = 5000;
    DWORD bytesReturned = 0;
    WSAIoctl(socket.native_handle(), SIO_KEEPALIVE_VALS,
             &ka, sizeof(ka), nullptr, 0, &bytesReturned, nullptr, nullptr);
#elif defined(__linux__)
    int fd = socket.native_handle();
    int idle = 10;
    int intvl = 5;
    int cnt = 3;
    setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, &idle, sizeof(idle));
    setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, &intvl, sizeof(intvl));
    setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, &cnt, sizeof(cnt));
#elif defined(__APPLE__)
    int fd = socket.native_handle();
    int idle = 10;
    int intvl = 5;
    int cnt = 3;
    setsockopt(fd, IPPROTO_TCP, TCP_KEEPALIVE, &idle, sizeof(idle));
    setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, &intvl, sizeof(intvl));
    setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, &cnt, sizeof(cnt));
#endif

    LOG_DEBUG("Control keepalive enabled (idle=10s, interval=5s, probes=3)");
}

Client::Client(asio::io_context& context,
    std::function<void(uint32_t type, const unsigned char* data, size_t size)>&& onPacket,
    std::function<void()>&& onConnectionDown)
    : m_context(context)
    , m_socket(context)
    , m_onPacket(std::move(onPacket))
    , m_onConnectionDown(std::move(onConnectionDown))
{
}

Client::~Client() {
    disconnect();
}

void Client::connect(const std::string& host, const std::string& port) {
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
        signalConnectResult(false);
        if (m_onConnectionDown)
            m_onConnectionDown();
        return;
    }

    if (!m_socket.is_open()) {
        std::error_code ec;
        m_socket.open(asio::ip::tcp::v4(), ec);
        if (ec) {
            LOG_ERROR("Control failed to open socket: {}", core::utilities::errorCodeForLog(ec));
            m_connecting = false;
            signalConnectResult(false);
            if (m_onConnectionDown)
                m_onConnectionDown();
            return;
        }
    }

    runConnect(host, portNumber);
}

bool Client::connectSync(const std::string& host, const std::string& port, int timeoutMs) {
    m_connectPromise = std::make_shared<std::promise<bool>>();
    auto future = m_connectPromise->get_future();

    connect(host, port);

    if (future.wait_for(std::chrono::milliseconds(timeoutMs)) == std::future_status::ready) {
        return future.get();
    }

    LOG_WARN("Control connectSync timed out after {}ms", timeoutMs);
    disconnect();
    return false;
}

void Client::signalConnectResult(bool success) {
    if (m_connectPromise) {
        try {
            m_connectPromise->set_value(success);
        }
        catch (const std::future_error&) {
            // Promise already satisfied — ignore
        }
        m_connectPromise.reset();
    }
}

void Client::runConnect(const std::string& host, uint16_t portNumber) {
    auto onConnected = [this](std::error_code errorCode, const asio::ip::tcp::endpoint&) {
        if (errorCode) {
            if (errorCode != asio::error::operation_aborted)
                LOG_ERROR("Control connect failed: {}", core::utilities::errorCodeForLog(errorCode));
            m_connecting = false;
            signalConnectResult(false);
            if (m_onConnectionDown) m_onConnectionDown();
            return;
        }
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
                signalConnectResult(false);
                if (m_onConnectionDown) m_onConnectionDown();
                return;
            }
            asio::async_connect(m_socket, results, std::move(onConnected));
        });
}

void Client::readHandshake() {
    asio::async_read(m_socket, asio::buffer(&m_handshakeIn, sizeof(uint64_t)),
        [this](std::error_code errorCode, std::size_t) {
            if (errorCode) {
                if (errorCode != asio::error::operation_aborted)
                    LOG_ERROR("Control handshake read error: {}", core::utilities::errorCodeForLog(errorCode));
                m_connecting = false;
                signalConnectResult(false);
                if (m_onConnectionDown) m_onConnectionDown();
                return;
            }

            m_handshakeOut = scramble(m_handshakeIn);
            writeHandshake();
        });
}

void Client::writeHandshake() {
    asio::async_write(m_socket, asio::buffer(&m_handshakeOut, sizeof(uint64_t)),
        [this](std::error_code errorCode, std::size_t) {
            if (errorCode) {
                if (errorCode != asio::error::operation_aborted)
                    LOG_ERROR("Control handshake write error: {}", core::utilities::errorCodeForLog(errorCode));
                m_connecting = false;
                signalConnectResult(false);
                if (m_onConnectionDown) m_onConnectionDown();
                return;
            }
            readHandshakeConfirmation();
        });
}

void Client::readHandshakeConfirmation() {
    asio::async_read(m_socket, asio::buffer(&m_handshakeConfirmation, sizeof(uint64_t)),
        [this](std::error_code errorCode, std::size_t) {
            if (errorCode) {
                if (errorCode != asio::error::operation_aborted)
                    LOG_ERROR("Control handshake confirmation read error: {}", core::utilities::errorCodeForLog(errorCode));
                m_connecting = false;
                signalConnectResult(false);
                if (m_onConnectionDown) m_onConnectionDown();
                return;
            }

            if (m_handshakeConfirmation != m_handshakeOut) {
                LOG_WARN("Control handshake validation failed");
                m_connecting = false;
                signalConnectResult(false);
                if (m_onConnectionDown) m_onConnectionDown();
                return;
            }
            m_connecting = false;
            m_connected = true;
            signalConnectResult(true);
            initializeAfterHandshake();
        });
}

void Client::initializeAfterHandshake() {
    configureKeepalive(m_socket);

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

void Client::processPacketQueue() {
    using namespace std::chrono_literals;
    while (!m_shuttingDown) {
        auto optionalPacket = m_inQueue.pop_for(500ms);
        if (!optionalPacket)
            continue;
        Packet packet = std::move(*optionalPacket);
        const unsigned char* data = packet.body.empty() ? nullptr : packet.body.data();
        size_t size = packet.body.size();
        if (m_onPacket)
            m_onPacket(packet.type, data, size);
    }
}

void Client::disconnect() {
    m_shuttingDown = true;
    m_connected = false;
    m_connecting = false;

    signalConnectResult(false);

    if (m_processThread.joinable()) {
        m_processThread.join();
    }

    asio::post(m_context, [this]() {
        if (m_socket.is_open()) {
            std::error_code errorCode;
            m_socket.shutdown(asio::ip::tcp::socket::shutdown_both, errorCode);
            m_socket.close(errorCode);
            if (errorCode)
                LOG_ERROR("Control socket close error: {}", core::utilities::errorCodeForLog(errorCode));
        }
    });

    // Give the posted close a moment to execute on the shared io thread
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    m_receiver.reset();
    m_sender.reset();
    m_outQueue.clear();
    m_inQueue.clear();
    m_shuttingDown = false;
}

bool Client::isConnected() const {
    return m_connected.load() && m_socket.is_open() && !m_shuttingDown;
}

bool Client::send(uint32_t type, const std::vector<unsigned char>& body) {
    return send(type, body.data(), body.size());
}

bool Client::send(uint32_t type, const void* data, size_t size) {
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
