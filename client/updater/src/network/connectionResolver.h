#pragma once

#include <string>
#include <functional>
#include <cstdint>

#include <asio.hpp>

namespace updater
{
	namespace network
	{
		class ConnectionResolver
		{
		public:
			ConnectionResolver(asio::io_context& context,
				asio::ip::tcp::socket& socket);

			void connect(const std::string& host, const uint16_t port,
				std::function<void()>&& onHandshakeComplete,
				std::function<void()>&& onError);

		private:
			void readHandshake();
			void readHandshakeConfirmation();
			void writeHandshake();

		private:
			asio::io_context& m_context;
			asio::ip::tcp::socket& m_socket;
			uint64_t m_handshakeOut = 0;
			uint64_t m_handshakeIn = 0;
			uint64_t m_handshakeConfirmation = 0;

			std::function<void()> m_onHandshakeComplete;
			std::function<void()> m_onError;
		};
	}
}
