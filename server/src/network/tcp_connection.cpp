#include "tcp_connection.h"
#include "utilities/logger.h"
#include "utilities/crypto.h"

#include <random>
#include <cstdint>

#if defined(_MSC_VER)
#include <cstdlib>
#endif

namespace server
{
    namespace network
    {
        using namespace server::utilities::crypto;

        TcpConnection::TcpConnection(
            asio::io_context& ctx,
            asio::ip::tcp::socket&& socket,
            std::function<void(OwnedTcpPacket&&)> onPacket,
            std::function<void(TcpConnectionPtr)> onDisconnected)
            : m_ctx(ctx)
            , m_socket(std::move(socket))
            , m_receiver(
                m_socket,
                [this](TcpPacket&& p) {
                    m_onPacket(OwnedTcpPacket{ shared_from_this(), std::move(p) });
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
                LOG_WARN("[TCP] Failed to enable keepalive: {}", ec.message());

            std::random_device rd;
            std::mt19937_64 gen(rd());
            std::uniform_int_distribution<uint64_t> dis;
            m_handshakeOut = dis(gen);

            writeHandshake();
        }

        void TcpConnection::sendPacket(const TcpPacket& packet) {
            m_outQueue.push(packet);
            if (m_outQueue.size() == 1)
                m_sender.send();
        }

        void TcpConnection::close() {
            TcpConnectionPtr self = shared_from_this();
            asio::post(m_ctx, [this, self]() {
                if (m_socket.is_open()) {
                    std::error_code ec;
                    m_socket.close(ec);
                    if (ec)
                        LOG_ERROR("[TCP] Socket close error: {}", ec.message());
                }
            });
        }

        asio::ip::tcp::endpoint TcpConnection::remoteEndpoint() const {
            std::error_code ec;
            auto ep = m_socket.remote_endpoint(ec);
            return ec ? asio::ip::tcp::endpoint{} : ep;
        }

        void TcpConnection::writeHandshake() {
            asio::async_write(m_socket, asio::buffer(&m_handshakeOut, sizeof(uint64_t)),
                [this](std::error_code ec, std::size_t) {
                    if (ec) {
                        if (ec == asio::error::operation_aborted)
                            return;
                        LOG_ERROR("[TCP] Handshake write error: {}", ec.message());
                        m_onDisconnected(shared_from_this());
                        return;
                    }

                    readHandshake();
                });
        }

        void TcpConnection::readHandshake() {
            asio::async_read(m_socket, asio::buffer(&m_handshakeIn, sizeof(uint64_t)),
                [this](std::error_code ec, std::size_t) {
                    if (ec) {
                        if (ec == asio::error::operation_aborted)
                            return;
                        LOG_ERROR("[TCP] Handshake read error: {}", ec.message());
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
                                LOG_ERROR("[TCP] Handshake confirmation write error: {}", ec2.message());
                                m_onDisconnected(shared_from_this());
                                return;
                            }
                            m_receiver.startReceiving();
                        });
                });
        }
    }
}
