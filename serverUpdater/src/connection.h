#pragma once

#include <iostream>
#include <variant>
#include <string>
#include <vector>
#include <filesystem>

#include "safedeque.h"
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
	void sendPacket(const Packet& packet);
	void sendFile(const std::filesystem::path& path);
	void close();

private:
	void writeHandshake();
	void readHandshake();

private:
	SafeDeque<std::variant<Packet, std::filesystem::path>> m_queue;

	uint64_t m_handshakeOut;
	uint64_t m_handshakeIn;

	asio::ip::tcp::socket m_socket;
	asio::io_context& m_asioContext;

	FilesSender m_filesSender;
	PacketsSender m_packetsSender;
	PacketsReceiver m_packetsReceiver;

	std::function<void(OwnedPacket&&)> m_queueReceivedPacket;
	std::function<void(std::shared_ptr<Connection>)> m_onDisconnected;
};
