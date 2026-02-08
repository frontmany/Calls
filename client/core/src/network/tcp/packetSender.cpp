#include "network/tcp/packetsSender.h"
#include "utilities/logger.h"
#include "utilities/errorCodeForLog.h"

namespace core::network::tcp {

PacketsSender::PacketsSender(
    asio::ip::tcp::socket& socket,
    core::utilities::SafeQueue<Packet>& queue,
    std::function<void()> onError)
    : m_socket(socket)
    , m_queue(queue)
    , m_onError(std::move(onError))
{
}

void PacketsSender::send() {
    writeHeader();
}

void PacketsSender::writeHeader() {
    const Packet* packet = m_queue.front_ptr();
    if (!packet)
        return;
    PacketHeader header;
    header.type = packet->type;
    header.bodySize = static_cast<uint32_t>(packet->body.size());
    asio::async_write(m_socket, asio::buffer(&header, sizeof(header)),
        [this, packet](std::error_code errorCode, std::size_t) {
            if (errorCode) {
                if (errorCode != asio::error::operation_aborted)
                    LOG_ERROR("Control write header error: {}", core::utilities::errorCodeForLog(errorCode));
                m_onError();
                return;
            }
            if (packet->body.empty()) {
                (void)m_queue.try_pop();
                resolveSending();
            }
            else {
                writeBody(packet);
            }
        });
}

void PacketsSender::writeBody(const Packet* packet) {
    asio::async_write(m_socket, asio::buffer(packet->body.data(), packet->body.size()),
        [this](std::error_code errorCode, std::size_t) {
            if (errorCode) {
                if (errorCode != asio::error::operation_aborted)
                    LOG_ERROR("Control write body error: {}", core::utilities::errorCodeForLog(errorCode));
                m_onError();
                return;
            }
            (void)m_queue.try_pop();
            resolveSending();
        });
}

void PacketsSender::resolveSending() {
    if (!m_queue.empty())
        writeHeader();
}

}
