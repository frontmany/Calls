#include "network/tcp/packetReceiver.h"
#include "constants/constant.h"
#include "utilities/logger.h"
#include "utilities/errorCodeForLog.h"

namespace server::network::tcp
{
    PacketsReceiver::PacketsReceiver(
        asio::ip::tcp::socket& socket,
        std::function<void(Packet&&)> onPacket,
        std::function<void()> onDisconnect)
        : m_socket(socket)
        , m_onPacket(std::move(onPacket))
        , m_onDisconnect(std::move(onDisconnect))
    {
    }

    void PacketsReceiver::startReceiving() {
        readHeader();
    }

    void PacketsReceiver::readHeader() {
        m_temporary.type = 0;
        m_temporary.bodySize = 0;
        m_temporary.body.clear();
        PacketHeader* hdr = m_temporary.headerPtrMut();
        asio::async_read(m_socket, asio::buffer(hdr, sizeof(PacketHeader)),
            [this](std::error_code ec, std::size_t) {
                if (ec) {
                    if (ec == asio::error::operation_aborted)
                        return;
                    LOG_DEBUG("[TCP] Read header error: {} - disconnecting", server::utilities::errorCodeForLog(ec));
                    m_onDisconnect();
                    return;
                }
                if (m_temporary.bodySize > 0) {
                    if (m_temporary.bodySize > server::constant::MAX_TCP_PACKET_BODY_SIZE_BYTES) {
                        LOG_WARN("[TCP] Packet body too large: {} bytes (max: {}), type: {} - disconnecting",
                            m_temporary.bodySize,
                            server::constant::MAX_TCP_PACKET_BODY_SIZE_BYTES,
                            m_temporary.type);
                        m_onDisconnect();
                        return;
                    }
                    m_temporary.body.resize(m_temporary.bodySize);
                    readBody();
                } else {
                    m_onPacket(std::move(m_temporary));
                    m_temporary = Packet{};
                    readHeader();
                }
            });
    }

    void PacketsReceiver::readBody() {
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
                m_temporary = Packet{};
                readHeader();
            });
    }
}
