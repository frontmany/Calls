#include "connection.h"
#include "utilities/logger.h"
#include "utilities/crypto.h"
#include "utilities/errorCodeForLog.h"

#include <random>
#include <cstdint>

#if defined(_MSC_VER)
#include <cstdlib>
#endif

namespace server::network::tcp
{
    Connection::Connection(
        asio::io_context& ctx,
        asio::ip::tcp::socket&& socket,
        std::function<void(OwnedPacket&&)> onPacket,
        std::function<void(Connection)> onDisconnected)
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
        std::error_code ec;
        m_socket.set_option(asio::ip::tcp::socket::keep_alive(true), ec);
        if (ec)
            LOG_WARN("[TCP] Failed to enable keepalive: {}", server::utilities::errorCodeForLog(ec));

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
                if (scramble(m_handshakeOut) != m_handshakeIn) {
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
