#include "tcp_packets_sender.h"
#include "utilities/logger.h"

namespace server
{
    namespace network
    {
        TcpPacketsSender::TcpPacketsSender(
            asio::ip::tcp::socket& socket,
            utilities::SafeQueue<TcpPacket>& queue,
            std::function<void()> onError)
            : m_socket(socket)
            , m_queue(queue)
            , m_onError(std::move(onError))
        {
        }

        void TcpPacketsSender::send() {
            writeHeader();
        }

        void TcpPacketsSender::writeHeader() {
            const TcpPacket* p = m_queue.front_ptr() ? &m_queue.unsafe_front() : nullptr;
            if (!p)
                return;
            TcpPacketHeader hdr;
            hdr.type = p->type;
            hdr.bodySize = static_cast<uint32_t>(p->body.size());
            asio::async_write(m_socket, asio::buffer(&hdr, sizeof(hdr)),
                [this, p](std::error_code ec, std::size_t) {
                    if (ec) {
                        if (ec != asio::error::operation_aborted)
                            LOG_ERROR("[TCP] Write header error: {}", ec.message());
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

        void TcpPacketsSender::writeBody(const TcpPacket* packet) {
            asio::async_write(m_socket, asio::buffer(packet->body.data(), packet->body.size()),
                [this](std::error_code ec, std::size_t) {
                    if (ec) {
                        if (ec != asio::error::operation_aborted)
                            LOG_ERROR("[TCP] Write body error: {}", ec.message());
                        m_onError();
                        return;
                    }
                    m_queue.try_pop();
                    resolveSending();
                });
        }

        void TcpPacketsSender::resolveSending() {
            if (!m_queue.empty())
                writeHeader();
        }
    }
}
