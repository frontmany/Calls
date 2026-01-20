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
			m_queue.push(packet);
			
			if (m_queue.size() == 1) {
				writeHeader();
			}
		}

		void PacketSender::writeHeader()
		{
			auto* packetPtr = m_queue.front_ptr();
			if (!packetPtr) {
				return;
			}

			asio::async_write(
				m_socket,
				asio::buffer(&packetPtr->header(), Packet::sizeOfHeader()),
				[this, packetPtr](std::error_code ec, std::size_t length) {
					if (ec) {
						if (ec != asio::error::connection_reset && ec != asio::error::operation_aborted) {
							m_onError();
						}
					}
					else {
						if (packetPtr->body().size() > 0) {
							writeBody(packetPtr);
						}
						else {
							m_queue.try_pop();
							resolveSending();
						}
					}
				}
			);
		}

		void PacketSender::writeBody(const Packet* packet)
		{
			asio::async_write(
				m_socket,
				asio::buffer(packet->body().data(), packet->body().size()),
				[this](std::error_code ec, std::size_t length) {
					if (ec) {
						if (ec != asio::error::connection_reset && ec != asio::error::operation_aborted) {
							m_onError();
						}
					}
					else {
						m_queue.try_pop();
						resolveSending();
					}
				}
			);
		}

		void PacketSender::resolveSending()
		{
			if (!m_queue.empty()) {
				writeHeader();
			}
		}
	}
}
