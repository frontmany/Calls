#include "network/connectionResolver.h"
#include "../utilities/utilities.h"

namespace updater
{
	namespace network
	{
		ConnectionResolver::ConnectionResolver(asio::io_context& context,
			asio::ip::tcp::socket& socket)
			: m_context(context),
			m_socket(socket)
		{
		}

		void ConnectionResolver::connect(const std::string& host, const uint16_t port,
			std::function<void()>&& onHandshakeComplete,
			std::function<void()>&& onError)
		{
			m_onHandshakeComplete = std::move(onHandshakeComplete);
			m_onError = std::move(onError);

			try {
				asio::ip::tcp::resolver resolver(m_context);
				const asio::ip::tcp::resolver::results_type& serverEndpoint = resolver.resolve(host, std::to_string(port));

				asio::async_connect(m_socket, serverEndpoint,
					[this](std::error_code ec, const asio::ip::tcp::endpoint& endpoint) {
						if (ec) {
							if (ec != asio::error::connection_reset && ec != asio::error::operation_aborted) {
								m_onError();
							}
						}
						else {
							readHandshake();
						}
					});
			}
			catch (std::exception& e) {
				m_onError();
			}
		}

		void ConnectionResolver::readHandshake()
		{
			asio::async_read(m_socket, asio::buffer(&m_handshakeIn, sizeof(uint64_t)),
				[this](std::error_code ec, std::size_t length) {
					if (ec) {
						m_onError();
					}
					else {
						m_handshakeOut = updater::utilities::scramble(m_handshakeIn);
						writeHandshake();
					}
				});
		}

		void ConnectionResolver::readHandshakeConfirmation()
		{
			asio::async_read(m_socket, asio::buffer(&m_handshakeConfirmation, sizeof(uint64_t)),
				[this](std::error_code ec, std::size_t length) {
					if (ec) {
						m_onError();
					}
					else {
						if (m_handshakeConfirmation == m_handshakeOut) {
							m_onHandshakeComplete();
						}
						else {
							m_onError();
						}
					}
				});
		}

		void ConnectionResolver::writeHandshake()
		{
			asio::async_write(m_socket, asio::buffer(&m_handshakeOut, sizeof(uint64_t)),
				[this](std::error_code ec, std::size_t length) {
					if (ec) {
						m_onError();
					}
					else {
						readHandshakeConfirmation();
					}
				});
		}
	}
}
