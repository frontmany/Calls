#include "packetsSender.h"
#include "filesSender.h"
#include "packet.h"

#include "utilities/logger.h"

using namespace serverUpdater;

namespace serverUpdater
{
PacketsSender::PacketsSender(asio::io_context& asioContext,
	asio::ip::tcp::socket& socket,
	utilities::SafeDeque<std::variant<Packet, std::filesystem::path>>& queue,
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
	writeHeader();
}

void PacketsSender::writeHeader() {
	auto& variant = m_queue.front();

	if (auto packet = std::get_if<Packet>(&variant)) {
		asio::async_write(
			m_socket,
			asio::buffer(&packet->header(), Packet::sizeOfHeader()),
			[this, packet](std::error_code ec, std::size_t length) {
				if (ec)
				{
					LOG_ERROR("Packet header write error: {}", ec.message());
					m_onError();
				}
				else
				{
					if (packet->body().size() > 0)
					{
						writeBody(packet);
					}
					else
					{
						m_queue.pop_front();
						resolveSending();
					}
				}
			}
		);
	}
}

void PacketsSender::writeBody(const Packet* packet) {
	asio::async_write(
		m_socket,
		asio::buffer(packet->body().data(), packet->body().size()),
		[this](std::error_code ec, std::size_t length) {
			if (ec) 
			{
				LOG_ERROR("Packet body write error: {}", ec.message());
				m_onError();
			}
			else 
			{
				LOG_TRACE("Packet sent successfully");
				m_queue.pop_front();
				resolveSending();
			}
		}
	);
}

void PacketsSender::resolveSending() {
	if (!m_queue.empty()) {
		auto& variant = m_queue.front();
		if (auto packet = std::get_if<Packet>(&variant)) {
			writeHeader();
		}
		else {
			m_filesSender.sendFile();
		}
	}
}
}
