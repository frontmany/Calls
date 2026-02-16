#include "network/tcp/packetSender.h"
#include "utilities/logger.h"
#include "utilities/errorCodeForLog.h"

namespace server::network::tcp
{
    PacketSender::PacketSender(
        asio::ip::tcp::socket& socket,
        utilities::SafeQueue<Packet>& queue,
        std::function<void()> onError)
        : m_socket(socket)
        , m_queue(queue)
        , m_onError(std::move(onError))
    {
    }

    void PacketSender::send() {
        writeHeader();
    }

    void PacketSender::writeHeader() {
        const Packet* p = m_queue.front_ptr() ? &m_queue.unsafe_front() : nullptr;
        if (!p) return;

        PacketHeader hdr;
        hdr.type = p->type;
        hdr.bodySize = static_cast<uint32_t>(p->body.size());
        asio::async_write(m_socket, asio::buffer(&hdr, sizeof(hdr)),
            [this, p](std::error_code ec, std::size_t) {
                if (ec) {
                    if (ec != asio::error::operation_aborted)
                        LOG_ERROR("[TCP] Write header error: {}", server::utilities::errorCodeForLog(ec));
                    m_onError();
                    return;
                }
                if (p->body.empty()) {
                    m_queue.try_pop();
                    resolveSending();
                } else {
                    writeBody(p);
                }
            });
    }

    void PacketSender::writeBody(const Packet* packet) {
        asio::async_write(m_socket, asio::buffer(packet->body.data(), packet->body.size()),
            [this](std::error_code ec, std::size_t) {
                if (ec) {
                    if (ec != asio::error::operation_aborted)
                        LOG_ERROR("[TCP] Write body error: {}", server::utilities::errorCodeForLog(ec));
                    m_onError();
                    return;
                }
                m_queue.try_pop();
                resolveSending();
            });
    }

    void PacketSender::resolveSending() {
        if (!m_queue.empty())
            writeHeader();
    }
}