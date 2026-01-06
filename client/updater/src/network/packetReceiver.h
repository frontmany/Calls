#pragma once

#include <functional>
#include <atomic>

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
				std::function<void()>&& onError
			);

			void startReceiving();
			void pause();
			void resume();

		private:
			void readHeader();
			void readBody();

		private:
			asio::ip::tcp::socket& m_socket;
			Packet m_temporaryPacket;

			std::function<void(Packet&&)> m_onPacketReceived;
			std::function<void()> m_onError;

			std::atomic_bool m_isPaused{ false };
			std::atomic_bool m_isReceiving{ false };
		};
	}
}
