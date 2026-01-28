#include "packetsReceiver.h"
#include "utilities/logger.h"
#include "utilities/errorCodeForLog.h"

using namespace serverUpdater;

namespace serverUpdater
{
PacketsReceiver::PacketsReceiver(asio::ip::tcp::socket& socket,
	std::function<void(Packet&&)>&& queueReceivedPacket,
	std::function<void()>&& onDisconnect)
	: m_socket(socket),
	m_queueReceivedPacket(std::move(queueReceivedPacket)),
	m_onDisconnect(std::move(onDisconnect))
{
}

void PacketsReceiver::startReceiving() {
	readHeader();
}

void PacketsReceiver::readHeader() {
	asio::async_read(m_socket, asio::buffer(&m_temporaryPacket.header_mut(), Packet::sizeOfHeader()),
		[this](std::error_code ec, std::size_t length) {
			if (ec) {
				if (ec == asio::error::operation_aborted) {
					// Operation was cancelled, likely due to graceful shutdown - ignore
					return;
				}
				// All other errors, including connection_reset, indicate client disconnect
				LOG_DEBUG("[SERVER] Connection error detected in readHeader: {} - disconnecting client", utilities::errorCodeForLog(ec));
				m_onDisconnect();
			}
			else { 
				LOG_TRACE("Received packet header, type: {}, size: {}", m_temporaryPacket.type(), m_temporaryPacket.size());
				if (m_temporaryPacket.size() > Packet::sizeOfHeader()) {
					m_temporaryPacket.body_mut().resize(m_temporaryPacket.size() - Packet::sizeOfHeader());
					readBody();
				}
				else {
					m_queueReceivedPacket(std::move(m_temporaryPacket));
					readHeader();
				}
			}
		});
}

void PacketsReceiver::readBody() {
	asio::async_read(m_socket, asio::buffer(m_temporaryPacket.body_mut().data(), m_temporaryPacket.body().size()),
		[this](std::error_code ec, std::size_t length) {
			if (ec) {
				if (ec == asio::error::operation_aborted) {
					// Operation was cancelled, likely due to graceful shutdown - ignore
					return;
				}
				// All other errors, including connection_reset, indicate client disconnect
				LOG_DEBUG("[SERVER] Connection error detected in readBody: {} - disconnecting client", utilities::errorCodeForLog(ec));
				m_onDisconnect();
			}
			else {
				m_queueReceivedPacket(std::move(m_temporaryPacket));
				readHeader();
			}
		});
}
}