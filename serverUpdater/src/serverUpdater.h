#pragma once

#include <filesystem>
#include <string>
#include <memory>
#include <vector>
#include <unordered_map>

#include "network/networkController.h"
#include "network/connection.h"
#include "network/packet.h"
#include "operationSystemType.h"

#include "json.hpp"

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

		// Helper methods for onUpdateAccepted
		std::vector<std::pair<std::filesystem::path, std::string>> parseClientFiles(const nlohmann::json& jsonObject);
		std::filesystem::path getOSSpecificPath(const std::filesystem::path& versionPath, OperationSystemType osType);
		std::unordered_map<std::filesystem::path, std::string> scanServerFiles(const std::filesystem::path& osSpecificPath);
		std::pair<std::vector<std::filesystem::path>, std::vector<std::filesystem::path>> 
			determineFilesToSync(const std::vector<std::pair<std::filesystem::path, std::string>>& clientFiles,
								 const std::unordered_map<std::filesystem::path, std::string>& serverFiles);
		nlohmann::json createUpdateMetadata(const std::vector<std::filesystem::path>& filesToSend,
											 const std::vector<std::filesystem::path>& filesToDelete,
											 const std::unordered_map<std::filesystem::path, std::string>& serverFiles,
											 const std::filesystem::path& osSpecificPath,
											 const std::string& version);
		void sendUpdateMetadata(ConnectionPtr connection, const nlohmann::json& metadata);
		void sendUpdateFiles(ConnectionPtr connection, const std::vector<std::filesystem::path>& filesToSend,
							 const std::filesystem::path& osSpecificPath);

	private:
		std::unique_ptr<NetworkController> m_networkController;
		std::filesystem::path m_versionsDirectory;
		uint16_t m_port;
	};
}
