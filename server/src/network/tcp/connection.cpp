#include "connection.h"
#include "utilities/logger.h"
#include "utilities/crypto.h"
#include "utilities/errorCodeForLog.h"

#include <random>
#include <cstdint>

#ifdef _WIN32
#include <mstcpip.h>
#elif defined(__linux__) || defined(__APPLE__)
#include <netinet/tcp.h>
#endif

namespace server::network::tcp
{
    static void configureKeepalive(asio::ip::tcp::socket& socket) {
        std::error_code ec;
        socket.set_option(asio::ip::tcp::socket::keep_alive(true), ec);
        if (ec) {
            LOG_WARN("[TCP] Failed to enable keepalive: {}", server::utilities::errorCodeForLog(ec));
            return;
        }

#ifdef _WIN32
        struct tcp_keepalive ka{};
        ka.onoff = 1;
        ka.keepalivetime = 2000;
        ka.keepaliveinterval = 1000;
        DWORD bytesReturned = 0;
        WSAIoctl(socket.native_handle(), SIO_KEEPALIVE_VALS,
                 &ka, sizeof(ka), nullptr, 0, &bytesReturned, nullptr, nullptr);
#elif defined(__linux__)
        int fd = socket.native_handle();
        int idle = 2;
        int intvl = 1;
        int cnt = 3;
        setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, &idle, sizeof(idle));
        setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, &intvl, sizeof(intvl));
        setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, &cnt, sizeof(cnt));
#elif defined(__APPLE__)
        int fd = socket.native_handle();
        int idle = 2;
        int intvl = 1;
        int cnt = 3;
        setsockopt(fd, IPPROTO_TCP, TCP_KEEPALIVE, &idle, sizeof(idle));
        setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, &intvl, sizeof(intvl));
        setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, &cnt, sizeof(cnt));
#endif

        LOG_DEBUG("[TCP] Keepalive enabled (idle=2s, interval=1s, probes=3)");
    }

    Connection::Connection(
        asio::io_context& ctx,
        asio::ip::tcp::socket&& socket,
        std::function<void(OwnedPacket&&)> onPacket,
        std::function<void(ConnectionPtr)> onDisconnected)
        : m_ctx(ctx)
        , m_socket(std::move(socket))
        , m_receiver(
            m_socket,
            [this](Packet&& p) {
                m_onPacket(OwnedPacket{ shared_from_this(), std::move(p) });
            },
            [this]() { m_onDisconnected(shared_from_this()); })
        , m_sender(
            m_socket,
            m_outQueue,
            [this]() { m_onDisconnected(shared_from_this()); })
        , m_onPacket(std::move(onPacket))
        , m_onDisconnected(std::move(onDisconnected))
    {
        configureKeepalive(m_socket);

        std::random_device rd;
        std::mt19937_64 gen(rd());
        std::uniform_int_distribution<uint64_t> dis;
        m_handshakeOut = dis(gen);

        writeHandshake();
    }

    void Connection::send(const Packet& packet) {
        m_outQueue.push(packet);
        if (m_outQueue.size() == 1)
            m_sender.send();
    }

    void Connection::close() {
        ConnectionPtr self = shared_from_this();
        asio::post(m_ctx, [this, self]() {
            if (m_socket.is_open()) {
                std::error_code ec;
                m_socket.close(ec);
                if (ec)
                    LOG_ERROR("[TCP] Socket close error: {}", server::utilities::errorCodeForLog(ec));
            }
        });
    }

    asio::ip::tcp::endpoint Connection::remoteEndpoint() const {
        std::error_code ec;
        auto ep = m_socket.remote_endpoint(ec);
        return ec ? asio::ip::tcp::endpoint{} : ep;
    }

    void Connection::writeHandshake() {
        asio::async_write(m_socket, asio::buffer(&m_handshakeOut, sizeof(uint64_t)),
            [this](std::error_code ec, std::size_t) {
                if (ec) {
                    if (ec == asio::error::operation_aborted)
                        return;
                    LOG_ERROR("[TCP] Handshake write error: {}", server::utilities::errorCodeForLog(ec));
                    m_onDisconnected(shared_from_this());
                    return;
                }

                readHandshake();
            });
    }

    void Connection::readHandshake() {
        asio::async_read(m_socket, asio::buffer(&m_handshakeIn, sizeof(uint64_t)),
            [this](std::error_code ec, std::size_t) {
                if (ec) {
                    if (ec == asio::error::operation_aborted)
                        return;
                    LOG_ERROR("[TCP] Handshake read error: {}", server::utilities::errorCodeForLog(ec));
                    m_onDisconnected(shared_from_this());
                    return;
                }
                if (server::utilities::crypto::scramble(m_handshakeOut) != m_handshakeIn) {
                    LOG_WARN("[TCP] Handshake validation failed");
                    m_onDisconnected(shared_from_this());
                    return;
                }

                asio::async_write(m_socket, asio::buffer(&m_handshakeIn, sizeof(uint64_t)),
                    [this](std::error_code ec2, std::size_t) {
                        if (ec2) {
                            if (ec2 == asio::error::operation_aborted)
                                return;
                            LOG_ERROR("[TCP] Handshake confirmation write error: {}", server::utilities::errorCodeForLog(ec2));
                            m_onDisconnected(shared_from_this());
                            return;
                        }
                        m_receiver.startReceiving();
                    });
            });
    }
}
