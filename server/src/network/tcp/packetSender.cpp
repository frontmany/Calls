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
        startNextIfNeeded();
    }

    void PacketSender::startNextIfNeeded() {
        if (m_current.has_value()) {
            return;
        }

        auto next = m_queue.try_pop();
        if (!next.has_value()) {
            return;
        }

        m_current = std::move(*next);
        writeHeader();
    }

    void PacketSender::writeHeader() {
        if (!m_current.has_value()) {
            startNextIfNeeded();
            return;
        }

        PacketHeader hdr;
        hdr.type = m_current->type;
        hdr.bodySize = static_cast<uint32_t>(m_current->body.size());
        asio::async_write(m_socket, asio::buffer(&hdr, sizeof(hdr)),
            [this](std::error_code ec, std::size_t) {
                if (ec) {
                    if (ec != asio::error::operation_aborted)
                        LOG_ERROR("[TCP] Write header error: {}", server::utilities::errorCodeForLog(ec));
                    m_onError();
                    return;
                }

                if (!m_current.has_value()) {
                    resolveSending();
                    return;
                }

                if (m_current->body.empty()) {
                    m_current.reset();
                    resolveSending();
                    return;
                }

                writeBody();
            });
    }

    void PacketSender::writeBody() {
        if (!m_current.has_value()) {
            resolveSending();
            return;
        }

        asio::async_write(m_socket, asio::buffer(m_current->body.data(), m_current->body.size()),
            [this](std::error_code ec, std::size_t) {
                if (ec) {
                    if (ec != asio::error::operation_aborted)
                        LOG_ERROR("[TCP] Write body error: {}", server::utilities::errorCodeForLog(ec));
                    m_onError();
                    return;
                }
                m_current.reset();
                resolveSending();
            });
    }

    void PacketSender::resolveSending() {
        startNextIfNeeded();
    }
}