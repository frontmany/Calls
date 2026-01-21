#include "network/packetReceiver.h"

namespace updater
{
	namespace network
	{
		PacketReceiver::PacketReceiver(asio::ip::tcp::socket& socket,
			std::function<void(Packet&&)>&& onPacketReceived,
			std::function<void()>&& onError,
			std::function<bool(uint32_t packetType)>&& shouldContinueReceive)
			: m_socket(socket),
			m_onPacketReceived(std::move(onPacketReceived)),
			m_onError(std::move(onError)),
			m_shouldContinueReceive(std::move(shouldContinueReceive))
		{
		}

		void PacketReceiver::startReceiving()
		{
			readHeader();
		}

		void PacketReceiver::readHeader()
		{
			asio::async_read(m_socket, asio::buffer(&m_temporaryPacket.header_mut(), Packet::sizeOfHeader()),
				[this](std::error_code ec, std::size_t length) {
					if (ec) {
						if (ec != asio::error::operation_aborted) {
							m_onError();
						}
					}
					else {
						if (m_temporaryPacket.size() > Packet::sizeOfHeader()) {
							m_temporaryPacket.body_mut().resize(m_temporaryPacket.size() - Packet::sizeOfHeader());
							readBody();
						}
						else {
							uint32_t packetType = m_temporaryPacket.type();
							m_onPacketReceived(std::move(m_temporaryPacket));
							bool shouldContinueReceive = m_shouldContinueReceive(packetType);
							
							if (shouldContinueReceive) {
								readHeader();
							}
						}
					}
				});
		}

		void PacketReceiver::readBody()
		{
			asio::async_read(m_socket, asio::buffer(m_temporaryPacket.body_mut().data(), m_temporaryPacket.body().size()),
				[this](std::error_code ec, std::size_t length) {
					if (ec) {
						if (ec != asio::error::connection_reset && ec != asio::error::operation_aborted) {
							m_onError();
						}
					}
					else {
						uint32_t packetType = m_temporaryPacket.type();
						m_onPacketReceived(std::move(m_temporaryPacket));
						bool shouldContinueReceive = m_shouldContinueReceive(packetType);
						
						if (shouldContinueReceive) {
							readHeader();
						}
					}
				});
		}
	}
}
