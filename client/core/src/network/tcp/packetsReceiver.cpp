#include "network/tcp/packetsReceiver.h"
#include "utilities/logger.h"

namespace core::network::tcp {

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
    PacketHeader* header = m_temporary.headerPtrMut();
    asio::async_read(m_socket, asio::buffer(header, sizeof(PacketHeader)),
        [this](std::error_code errorCode, std::size_t) {
            if (errorCode) {
                if (errorCode == asio::error::operation_aborted)
                    return;
                LOG_ERROR("Control read header error: {} - disconnecting", errorCode.message());
                m_onDisconnect();
                return;
            }
            if (m_temporary.bodySize > 0) {
                m_temporary.body.resize(m_temporary.bodySize);
                readBody();
            }
            else {
                m_onPacket(std::move(m_temporary));
                m_temporary = Packet{};
                readHeader();
            }
        });
}

void PacketsReceiver::readBody() {
    asio::async_read(m_socket, asio::buffer(m_temporary.body.data(), m_temporary.body.size()),
        [this](std::error_code errorCode, std::size_t) {
            if (errorCode) {
                if (errorCode == asio::error::operation_aborted)
                    return;
                LOG_ERROR("Control read body error: {} - disconnecting", errorCode.message());
                m_onDisconnect();
                return;
            }
            m_onPacket(std::move(m_temporary));
            m_temporary = Packet{};
            readHeader();
        });
}

}
