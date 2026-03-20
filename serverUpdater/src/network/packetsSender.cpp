#include "packetsSender.h"
#include "filesSender.h"
#include "packet.h"
#include "utilities/logger.h"
#include "utilities/errorCodeForLog.h"

using namespace serverUpdater;

namespace serverUpdater
{
PacketsSender::PacketsSender(asio::io_context& asioContext,
	asio::ip::tcp::socket& socket,
	utilities::SafeQueue<std::variant<Packet, std::filesystem::path>>& queue,
	FilesSender& filesSender,
	std::function<void()>&& onError)
	: m_socket(socket),
	m_queue(queue),
	m_filesSender(filesSender),
	m_asioContext(asioContext),
	m_onError(std::move(onError))
{
}

void PacketsSender::send() {
    bool expected = false;
    if (!m_sending.compare_exchange_strong(expected, true)) {
        return;
    }
    startNextIfNeeded();
}

bool PacketsSender::isSending() const {
    return m_sending.load();
}

void PacketsSender::startNextIfNeeded() {
    if (m_currentPacket.has_value()) {
        return;
    }
    auto front = m_queue.front();
    if (!front.has_value()) {
        m_sending.store(false);
        if (!m_queue.empty()) {
            send();
        }
        return;
    }
    if (auto packet = std::get_if<Packet>(&(*front))) {
        m_currentPacket = *packet;
        writeHeader();
        return;
    }
    m_sending.store(false);
    m_filesSender.sendFile();
}

void PacketsSender::writeHeader() {
    if (!m_currentPacket.has_value()) {
        resolveSending();
        return;
    }
    asio::async_write(
        m_socket,
        asio::buffer(&m_currentPacket->header(), Packet::sizeOfHeader()),
        [this](std::error_code ec, std::size_t) {
            if (ec)
            {
                LOG_ERROR("Packet header write error: {}", utilities::errorCodeForLog(ec));
                m_onError();
            }
            else
            {
                if (m_currentPacket.has_value() && !m_currentPacket->body().empty())
                {
                    writeBody();
                }
                else
                {
                    m_currentPacket.reset();
                    m_queue.try_pop();
                    resolveSending();
                }
            }
        }
    );
}

void PacketsSender::writeBody() {
    if (!m_currentPacket.has_value()) {
        resolveSending();
        return;
    }
	asio::async_write(
		m_socket,
		asio::buffer(m_currentPacket->body().data(), m_currentPacket->body().size()),
		[this](std::error_code ec, std::size_t) {
			if (ec) 
			{
				LOG_ERROR("Packet body write error: {}", utilities::errorCodeForLog(ec));
				m_onError();
			}
			else 
			{
				LOG_TRACE("Packet sent successfully");
                m_currentPacket.reset();
				m_queue.try_pop();
				resolveSending();
			}
		}
	);
}

void PacketsSender::resolveSending() {
	startNextIfNeeded();
}
}
