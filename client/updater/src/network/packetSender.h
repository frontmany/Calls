#pragma once

#include <functional>

#include "packet.h"

#include <asio.hpp>

namespace updater
{
	namespace network
	{
		class PacketSender
		{
		public:
			PacketSender(asio::ip::tcp::socket& socket,
				std::function<void()>&& onError
			);

			void sendPacket(const Packet& packet);

		private:
			void writeHeader(const Packet& packet);
			void writeBody(const Packet& packet);

		private:
			asio::ip::tcp::socket& m_socket;
			std::function<void()> m_onError;
		};
	}
}
