#include "tcp_control_client.h"
#include "tcp_packets_receiver.h"
#include "tcp_packets_sender.h"
#include "utilities/logger.h"
#include "utilities/crypto.h"

#include <array>
#include <chrono>
#include <cstdint>
#include <thread>

#if defined(_MSC_VER)
#include <cstdlib>
#endif

namespace core
{
    namespace network
    {
        using namespace core::utilities::crypto;
        TcpControlClient::TcpControlClient(OnControlPacket onControlPacket, std::function<void()> onConnectionDown)
            : m_socket(m_context)
            , m_onControlPacket(std::move(onControlPacket))
            , m_onConnectionDown(std::move(onConnectionDown))
        {
        }

        TcpControlClient::~TcpControlClient() {
            disconnect();
        }

        void TcpControlClient::connect(const std::string& host, const std::string& tcpPort) {
            if (m_connecting.exchange(true)) {
                LOG_WARN("[TCP] Already connecting/connected");
                return;
            }
            m_shuttingDown = false;

            uint16_t port = 0;
            try {
                port = static_cast<uint16_t>(std::stoul(tcpPort));
            }
            catch (...) {
                LOG_ERROR("[TCP] Invalid port: {}", tcpPort);
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

            runConnect(host, port);
        }

        void TcpControlClient::runConnect(const std::string& host, uint16_t port) {
            auto onConnected = [this](std::error_code ec2, const asio::ip::tcp::endpoint&) {
                if (ec2) {
                    if (ec2 != asio::error::operation_aborted)
                        LOG_ERROR("[TCP] Connect failed: {}", ec2.message());
                    m_connecting = false;
                    if (m_onConnectionDown) m_onConnectionDown();
                    return;
                }
                std::error_code sec;
                m_socket.set_option(asio::ip::tcp::socket::keep_alive(true), sec);
                if (sec)
                    LOG_WARN("[TCP] Keepalive failed: {}", sec.message());
                readHandshake();
            };

            std::error_code ec;
            asio::ip::address addr = asio::ip::make_address(host, ec);
            if (!ec) {
                std::array<asio::ip::tcp::endpoint, 1> eps = { asio::ip::tcp::endpoint(addr, port) };
                asio::async_connect(m_socket, eps, std::move(onConnected));
                return;
            }

            asio::ip::tcp::resolver resolver(m_context);
            resolver.async_resolve(host, std::to_string(port),
                [this, host, port, onConnected = std::move(onConnected)](
                    std::error_code ec, asio::ip::tcp::resolver::results_type results) mutable {
                    if (ec) {
                        LOG_ERROR("[TCP] Resolve {}:{} failed: {}", host, port, ec.message());
                        m_connecting = false;
                        if (m_onConnectionDown) m_onConnectionDown();
                        return;
                    }
                    asio::async_connect(m_socket, results, std::move(onConnected));
                });
        }

        void TcpControlClient::readHandshake() {
            asio::async_read(m_socket, asio::buffer(&m_handshakeIn, sizeof(uint64_t)),
                [this](std::error_code ec, std::size_t) {
                    if (ec) {
                        if (ec != asio::error::operation_aborted)
                            LOG_ERROR("[TCP] Handshake read error: {}", ec.message());
                        m_connecting = false;
                        if (m_onConnectionDown) m_onConnectionDown();
                        return;
                    }

                    m_handshakeOut = scramble(m_handshakeIn);
                    writeHandshake();
                });
        }

        void TcpControlClient::writeHandshake() {
            asio::async_write(m_socket, asio::buffer(&m_handshakeOut, sizeof(uint64_t)),
                [this](std::error_code ec, std::size_t) {
                    if (ec) {
                        if (ec != asio::error::operation_aborted)
                            LOG_ERROR("[TCP] Handshake write error: {}", ec.message());
                        m_connecting = false;
                        if (m_onConnectionDown) m_onConnectionDown();
                        return;
                    }
                    readHandshakeConfirmation();
                });
        }

        void TcpControlClient::readHandshakeConfirmation() {
            asio::async_read(m_socket, asio::buffer(&m_handshakeConfirmation, sizeof(uint64_t)),
                [this](std::error_code ec, std::size_t) {
                    if (ec) {
                        if (ec != asio::error::operation_aborted)
                            LOG_ERROR("[TCP] Handshake confirmation read error: {}", ec.message());
                        m_connecting = false;
                        if (m_onConnectionDown) m_onConnectionDown();
                        return;
                    }

                    if (m_handshakeConfirmation != m_handshakeOut) {
                        LOG_WARN("[TCP] Handshake validation failed");
                        m_connecting = false;
                        if (m_onConnectionDown) m_onConnectionDown();
                        return;
                    }
                    m_connecting = false;
                    m_connected = true;
                    initAfterHandshake();
                });
        }

        void TcpControlClient::initAfterHandshake() {
            m_receiver = std::make_unique<TcpPacketsReceiver>(m_socket,
                [this](TcpPacket&& p) { m_inQueue.push(std::move(p)); },
                [this]() {
                    if (!m_shuttingDown && m_connected.exchange(false)) {
                        LOG_WARN("[TCP] Disconnected (receive)");
                        if (m_onConnectionDown) m_onConnectionDown();
                    }
                });

            m_sender = std::make_unique<TcpPacketsSender>(m_socket, m_outQueue,
                [this]() {
                    if (!m_shuttingDown && m_connected.exchange(false)) {
                        LOG_WARN("[TCP] Disconnected (send)");
                        if (m_onConnectionDown) m_onConnectionDown();
                    }
                });

            m_processThread = std::thread([this]() { processPacketQueue(); });

            m_receiver->startReceiving();

            LOG_INFO("[TCP] Control channel connected");
        }

        void TcpControlClient::processPacketQueue() {
            using namespace std::chrono_literals;
            while (!m_shuttingDown) {
                auto opt = m_inQueue.pop_for(500ms);
                if (!opt)
                    continue;
                TcpPacket p = std::move(*opt);
                const unsigned char* data = p.body.empty() ? nullptr : p.body.data();
                size_t size = p.body.size();
                if (m_onControlPacket)
                    m_onControlPacket(p.type, data, size);
            }
        }

        void TcpControlClient::disconnect() {
            m_shuttingDown = true;
            m_connected = false;
            m_connecting = false;

            if (m_processThread.joinable()) {
                m_processThread.join();
            }

            asio::post(m_context, [this]() {
                if (m_socket.is_open()) {
                    std::error_code ec;
                    m_socket.close(ec);
                    if (ec)
                        LOG_ERROR("[TCP] Socket close error: {}", ec.message());
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

        bool TcpControlClient::isConnected() const {
            return m_connected.load() && m_socket.is_open() && !m_shuttingDown;
        }

        bool TcpControlClient::send(uint32_t type, const std::vector<unsigned char>& body) {
            return send(type, body.data(), body.size());
        }

        bool TcpControlClient::send(uint32_t type, const void* data, size_t size) {
            if (!m_sender || !isConnected())
                return false;
            TcpPacket p;
            p.type = type;
            p.bodySize = static_cast<uint32_t>(size);
            if (size > 0 && data) {
                const auto* bytes = static_cast<const uint8_t*>(data);
                p.body.assign(bytes, bytes + size);
            }
            bool wasEmpty = m_outQueue.empty();
            m_outQueue.push(std::move(p));
            if (wasEmpty)
                m_sender->send();
            return true;
        }
    }
}
