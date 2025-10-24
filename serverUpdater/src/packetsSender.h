#pragma once

#include "net_safeDeque.h"
#include "asio.hpp"


namespace net {
	class PacketsSender {
	public:
		PacketsSender(asio::io_context* asioContext,
			asio::ip::tcp::socket* socket, 
			std::function<void(std::error_code, Packet&&)> onSendPacketError,
			std::function<void()> onDisconnect,
			std::function<void(Packet&&)> onPacketSent
		);

		void send(const Packet& packet);

	private:
		void writeHeader();
		void writeBody();

	private:
		asio::ip::tcp::socket* m_socket = nullptr;
		asio::io_context* m_asioContext = nullptr;

		SafeDeque<Packet> m_safeDequeOutgoingPackets;

		std::function<void(std::error_code, Packet&&)> m_onSendPacketError;
		std::function<void()> m_onDisconnect;
		std::function<void(Packet&&)> m_onPacketSent;
	};
}

