#pragma once

#include <functional>

#include "packet.h"

#include <asio.hpp>

namespace updater
{
	namespace network
	{
		class PacketReceiver
		{
		public:
			PacketReceiver(asio::ip::tcp::socket& socket,
				std::function<void(Packet&&)>&& onPacketReceived,
				std::function<void()>&& onError,
				std::function<bool(uint32_t packetType)>&& shouldContinueReceive
			);

			void startReceiving();

		private:
			void readHeader();
			void readBody();

		private:
			asio::ip::tcp::socket& m_socket;
			Packet m_temporaryPacket;

			std::function<void(Packet&&)> m_onPacketReceived;
			std::function<void()> m_onError;
			std::function<bool(uint32_t packetType)> m_shouldContinueReceive;
		};
	}
}
