#include "packetsSender.h"
#include "packet.h"

PacketsSender::PacketsSender(asio::io_context& asioContext,
	asio::ip::tcp::socket& socket,
	std::function<void()>&& onError)
	: m_socket(socket),
	m_asioContext(asioContext),
	m_onError(std::move(onError))
{
}

void PacketsSender::send(const Packet& packet) {
	asio::post(m_asioContext, [this, packet]() {
		bool allowed = m_queue.empty();

		m_queue.push_back(packet);

		if (allowed) 
			writeHeader();
	});
}

void PacketsSender::writeHeader() {
	asio::async_write(
		m_socket,
		asio::buffer(&m_queue.front().header(), Packet::sizeOfHeader()),
		[this](std::error_code ec, std::size_t length) {
			if (ec)
			{
				m_onError();
			}
			else
			{
				if (m_queue.front().body().size() > 0)
				{
					writeBody();
				}
				else
				{
					if (!m_queue.empty())
						writeHeader();
				}
			}
		}
	);
}

void PacketsSender::writeBody() {
	asio::async_write(
		m_socket,
		asio::buffer(m_queue.front().body().data(), m_queue.front().body().size()),
		[this](std::error_code ec, std::size_t length) {
			if (ec) 
			{
				m_onError();
			}
			else 
			{
				if (!m_queue.empty())
					writeHeader();
			}
		}
	);
}


