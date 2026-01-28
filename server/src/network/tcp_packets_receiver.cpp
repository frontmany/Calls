#include "tcp_packets_receiver.h"
#include "utilities/logger.h"
#include "utilities/errorCodeForLog.h"

namespace server
{
    namespace network
    {
        TcpPacketsReceiver::TcpPacketsReceiver(
            asio::ip::tcp::socket& socket,
            std::function<void(TcpPacket&&)> onPacket,
            std::function<void()> onDisconnect)
            : m_socket(socket)
            , m_onPacket(std::move(onPacket))
            , m_onDisconnect(std::move(onDisconnect))
        {
        }

        void TcpPacketsReceiver::startReceiving() {
            readHeader();
        }

        void TcpPacketsReceiver::readHeader() {
            m_temporary.type = 0;
            m_temporary.bodySize = 0;
            m_temporary.body.clear();
            TcpPacketHeader* hdr = m_temporary.headerPtrMut();
            asio::async_read(m_socket, asio::buffer(hdr, sizeof(TcpPacketHeader)),
                [this](std::error_code ec, std::size_t) {
                    if (ec) {
                        if (ec == asio::error::operation_aborted)
                            return;
                        LOG_DEBUG("[TCP] Read header error: {} - disconnecting", server::utilities::errorCodeForLog(ec));
                        m_onDisconnect();
                        return;
                    }
                    if (m_temporary.bodySize > 0) {
                        m_temporary.body.resize(m_temporary.bodySize);
                        readBody();
                    } else {
                        m_onPacket(std::move(m_temporary));
                        m_temporary = TcpPacket{};
                        readHeader();
                    }
                });
        }

        void TcpPacketsReceiver::readBody() {
            asio::async_read(m_socket, asio::buffer(m_temporary.body.data(), m_temporary.body.size()),
                [this](std::error_code ec, std::size_t) {
                    if (ec) {
                        if (ec == asio::error::operation_aborted)
                            return;
                        LOG_DEBUG("[TCP] Read body error: {} - disconnecting", server::utilities::errorCodeForLog(ec));
                        m_onDisconnect();
                        return;
                    }
                    m_onPacket(std::move(m_temporary));
                    m_temporary = TcpPacket{};
                    readHeader();
                });
        }
    }
}
