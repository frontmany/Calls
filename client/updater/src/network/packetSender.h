#pragma once

#include <functional>

#include "packet.h"
#include "../utilities/safeQueue.h"

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
			void writeHeader();
			void writeBody(const Packet* packet);
			void resolveSending();

		private:
			utilities::SafeQueue<Packet> m_queue;
			asio::ip::tcp::socket& m_socket;
			std::function<void()> m_onError;
		};
	}
}
