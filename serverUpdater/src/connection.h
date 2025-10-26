#pragma once

#include <iostream>
#include <string>
#include <vector>

#include "safedeque.h"
#include "fileData.h"
#include "filesSender.h"
#include "packetsSender.h"
#include "packetsReceiver.h"

#include "asio.hpp"

class Packet;

class Connection : public std::enable_shared_from_this<Connection> {
public:
	Connection(
		asio::io_context& asioContext,
		asio::ip::tcp::socket&& socket,
		std::function<void(OwnedPacket&&)>&& queueReceivedPacket,
		std::function<void(std::shared_ptr<Connection>)>&& onDisconnected
	);

	~Connection();

	void sendUpdateRequiredPacket(const Packet& packet);
	void sendUpdatePossiblePacket(const Packet& packet);
	void sendUpdateNotNeededPacket(const Packet& packet);
	void sendUpdate(const Packet& packet, const std::vector<std::filesystem::path>& paths);
	void close();

private:
	asio::ip::tcp::socket m_socket;
	asio::io_context& m_asioContext;

	FilesSender m_filesSender;
	PacketsSender m_packetsSender;
	PacketsReceiver m_packetsReceiver;

	std::function<void(OwnedPacket&&)> m_queueReceivedPacket;
	std::function<void(std::shared_ptr<Connection>)> m_onDisconnected;
};
