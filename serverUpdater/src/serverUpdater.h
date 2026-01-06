#pragma once

#include <filesystem>
#include <string>
#include <memory>

#include "network/networkController.h"
#include "network/connection.h"
#include "network/packet.h"

namespace serverUpdater
{
	class ServerUpdater
	{
	public:
		ServerUpdater(uint16_t port, const std::filesystem::path& versionsDirectory);
		~ServerUpdater();

		void start();
		void stop();

	private:
		std::pair<std::filesystem::path, std::string> findLatestVersion();
		void onUpdatesCheck(ConnectionPtr connection, Packet&& packet);
		void onUpdateAccepted(ConnectionPtr connection, Packet&& packet);

	private:
		std::unique_ptr<NetworkController> m_networkController;
		std::filesystem::path m_versionsDirectory;
		uint16_t m_port;
	};
}
