#include "net_packetsSender.h"
#include "net_packet.h"

namespace net {

	PacketsSender::PacketsSender(asio::io_context* asioContext,
		asio::ip::tcp::socket* socket, 
		std::function<void(std::error_code, Packet&&)> onSendPacketError,
		std::function<void()> onDisconnect,
		std::function<void(Packet&&)> onPacketSent)
		: m_socket(socket),
		m_asioContext(asioContext),
		m_onSendPacketError(onSendPacketError),
		m_onDisconnect(onDisconnect),
		m_onPacketSent(onPacketSent)
	{
	}

	void PacketsSender::send(const Packet& packet) {
		asio::post(*m_asioContext, [this, packet]() {
			bool isAbleToWrite = m_safeDequeOutgoingPackets.empty();

			m_safeDequeOutgoingPackets.push_back(packet);

			if (isAbleToWrite) {
				writeHeader();
			}
		});
	}

	void PacketsSender::writeHeader() {
		asio::async_write(
			*m_socket,
			asio::buffer(&m_safeDequeOutgoingPackets.front().header(), Packet::sizeOfHeader()),
			[this](std::error_code ec, std::size_t length) {
				if (ec)
				{
					if (ec != asio::error::connection_reset) {
						m_onSendPacketError(ec, m_safeDequeOutgoingPackets.pop_front());

					}

					m_onDisconnect();
				}
				else
				{
					if (m_safeDequeOutgoingPackets.front().body().size() > 0)
					{
						writeBody();
					}
					else
					{
						m_onPacketSent(std::move(m_safeDequeOutgoingPackets.pop_front()));

						if (!m_safeDequeOutgoingPackets.empty())
						{
							writeHeader();
						}
					}
				}
			}
		);
	}

	void PacketsSender::writeBody() {
		asio::async_write(
			*m_socket,
			asio::buffer(m_safeDequeOutgoingPackets.front().body().data(), m_safeDequeOutgoingPackets.front().body().size()),
			[this](std::error_code ec, std::size_t length) {
				if (ec) {
					if (ec != asio::error::connection_reset) {
						m_onSendPacketError(ec, m_safeDequeOutgoingPackets.pop_front());
					}

					m_onDisconnect();
				}
				else {
					m_onPacketSent(std::move(m_safeDequeOutgoingPackets.pop_front()));

					if (!m_safeDequeOutgoingPackets.empty())
					{
						writeHeader();
					}
				}
			}
		);
	}
}

