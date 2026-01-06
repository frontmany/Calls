#include "network/packetSender.h"

namespace updater
{
	namespace network
	{
		PacketSender::PacketSender(asio::ip::tcp::socket& socket,
			std::function<void()>&& onError)
			: m_socket(socket),
			m_onError(std::move(onError))
		{
		}

		void PacketSender::sendPacket(const Packet& packet)
		{
			writeHeader(packet);
		}

		void PacketSender::writeHeader(const Packet& packet)
		{
			asio::async_write(
				m_socket,
				asio::buffer(&packet.header(), Packet::sizeOfHeader()),
				[this, packet](std::error_code ec, std::size_t length) {
					if (ec) {
						if (ec != asio::error::connection_reset && ec != asio::error::operation_aborted) {
							m_onError();
						}
					}
					else {
						if (packet.body().size() > 0) {
							writeBody(packet);
						}
					}
				}
			);
		}

		void PacketSender::writeBody(const Packet& packet)
		{
			asio::async_write(
				m_socket,
				asio::buffer(packet.body().data(), packet.body().size()),
				[this](std::error_code ec, std::size_t length) {
					if (ec) {
						if (ec != asio::error::connection_reset && ec != asio::error::operation_aborted) {
							m_onError();
						}
					}
				}
			);
		}
	}
}
