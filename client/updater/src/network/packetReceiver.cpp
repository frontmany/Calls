#include "network/packetReceiver.h"
#include <atomic>

namespace updater
{
	namespace network
	{
		PacketReceiver::PacketReceiver(asio::ip::tcp::socket& socket,
			std::function<void(Packet&&)>&& onPacketReceived,
			std::function<void()>&& onError)
			: m_socket(socket),
			m_onPacketReceived(std::move(onPacketReceived)),
			m_onError(std::move(onError))
		{
		}

		void PacketReceiver::startReceiving()
		{
			m_isReceiving = true;
			m_isPaused = false;
			readHeader();
		}

		void PacketReceiver::pause()
		{
			m_isPaused = true;
		}

		void PacketReceiver::resume()
		{
			if (m_isPaused && m_isReceiving) {
				m_isPaused = false;
				readHeader();
			}
		}

		void PacketReceiver::readHeader()
		{
			if (m_isPaused) {
				return;
			}

			asio::async_read(m_socket, asio::buffer(&m_temporaryPacket.header_mut(), Packet::sizeOfHeader()),
				[this](std::error_code ec, std::size_t length) {
					if (m_isPaused) {
						return;
					}

					if (ec) {
						if (ec != asio::error::connection_reset && ec != asio::error::operation_aborted) {
							m_onError();
						}
					}
					else {
						if (m_temporaryPacket.size() > Packet::sizeOfHeader()) {
							m_temporaryPacket.body_mut().resize(m_temporaryPacket.size() - Packet::sizeOfHeader());
							readBody();
						}
						else {
							m_onPacketReceived(std::move(m_temporaryPacket));
							if (!m_isPaused) {
								readHeader();
							}
						}
					}
				});
		}

		void PacketReceiver::readBody()
		{
			if (m_isPaused) {
				return;
			}

			asio::async_read(m_socket, asio::buffer(m_temporaryPacket.body_mut().data(), m_temporaryPacket.body().size()),
				[this](std::error_code ec, std::size_t length) {
					if (m_isPaused) {
						return;
					}

					if (ec) {
						if (ec != asio::error::connection_reset && ec != asio::error::operation_aborted) {
							m_onError();
						}
					}
					else {
						m_onPacketReceived(std::move(m_temporaryPacket));
						if (!m_isPaused) {
							readHeader();
						}
					}
				});
		}
	}
}
