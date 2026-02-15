#include "serverUpdater.h"

#include <unordered_map>
#include <unordered_set>
#include <fstream>

#include "operationSystemType.h"
#include "jsonType.h"
#include "checkResult.h"
#include "packetType.h"
#include "version.h"
#include "utilities/logger.h"
#include "utilities/utilities.h"

using namespace serverUpdater;
using namespace serverUpdater::utilities;

namespace serverUpdater
{
	constexpr const char* MAJOR_UPDATE = "major";
	constexpr const char* MINOR_UPDATE = "minor";

	ServerUpdater::ServerUpdater(uint16_t port, const std::filesystem::path& versionsDirectory)
		: m_port(port),
		m_versionsDirectory(versionsDirectory)
	{
		m_networkController = std::make_unique<NetworkController>(port,
			[this](ConnectionPtr connection, Packet&& packet) {
				onUpdatesCheck(connection, std::move(packet));
			},
			[this](ConnectionPtr connection, Packet&& packet) {
				onUpdateAccepted(connection, std::move(packet));
			});
	}

	ServerUpdater::~ServerUpdater()
	{
		stop();
	}

	void ServerUpdater::start()
	{
		if (std::filesystem::exists(m_versionsDirectory)) {
			for (const auto& entry : std::filesystem::directory_iterator(m_versionsDirectory)) {
				if (entry.is_directory()) {
				}
			}
		}

		LOG_INFO("[SERVER] Server started on port {}", m_port);
		m_networkController->start();
	}

	void ServerUpdater::stop()
	{
		if (m_networkController) {
			m_networkController->stop();
		}
	}

	std::pair<std::filesystem::path, std::string> ServerUpdater::findLatestVersion()
	{
		std::string latestVersion;
		std::filesystem::path latestVersionPath;

		LOG_DEBUG("Starting search for latest version in directory: {}", m_versionsDirectory.string());

		if (!std::filesystem::exists(m_versionsDirectory)) {
			LOG_ERROR("Versions directory does not exist: {}", m_versionsDirectory.string());
			return std::make_pair(latestVersionPath, latestVersion);
		}

		if (!std::filesystem::is_directory(m_versionsDirectory)) {
			LOG_ERROR("Versions path is not a directory: {}", m_versionsDirectory.string());
			return std::make_pair(latestVersionPath, latestVersion);
		}

		bool foundAnyVersions = false;
		int directoriesChecked = 0;
		int validVersionFiles = 0;

		for (const auto& entry : std::filesystem::directory_iterator(m_versionsDirectory)) {
			if (entry.is_directory()) {
				directoriesChecked++;
				std::filesystem::path versionJsonPath = entry.path() / "version.json";
				LOG_DEBUG("Checking directory: {}, version.json path: {}",
					entry.path().string(), versionJsonPath.string());

				if (std::filesystem::exists(versionJsonPath) &&
					std::filesystem::is_regular_file(versionJsonPath)) {

					try {
						std::ifstream file(versionJsonPath);
						nlohmann::json versionJson = nlohmann::json::parse(file);

						std::string currentVersion = versionJson[VERSION].get<std::string>();
						std::string updateType = versionJson[UPDATE_TYPE].get<std::string>();
						validVersionFiles++;

						LOG_DEBUG("Found version: {} (type: {}) in {}",
							currentVersion, updateType, entry.path().string());

						if (latestVersion.empty()) {
							latestVersion = currentVersion;
							latestVersionPath = entry.path();
							LOG_DEBUG("Setting as initial latest version: {}", currentVersion);
						}

						if (currentVersion > latestVersion) {
							LOG_DEBUG("New latest version found: {} (previous: {})",
								currentVersion, latestVersion);
							latestVersion = currentVersion;
							latestVersionPath = entry.path();
						}

						foundAnyVersions = true;
					}
					catch (const std::exception& e) {
						LOG_ERROR("Error parsing version.json in {}: {}",
							entry.path().string(), e.what());
					}
				}
				else {
					LOG_DEBUG("version.json not found or not a regular file in: {}",
						entry.path().string());
				}
			}
		}

		if (foundAnyVersions) {
			LOG_INFO("Latest version search completed: found version {} in {}",
				latestVersion, latestVersionPath.string());
			LOG_DEBUG("Directories checked: {}, valid version files: {}",
				directoriesChecked, validVersionFiles);
		}
		else {
			LOG_WARN("No valid versions found in directory: {}", m_versionsDirectory.string());
			LOG_DEBUG("Checked {} directories, found {} valid version files",
				directoriesChecked, validVersionFiles);
		}

		return std::make_pair(latestVersionPath, latestVersion);
	}

	void ServerUpdater::onUpdatesCheck(ConnectionPtr connection, Packet&& packet)
	{
		nlohmann::json jsonObject = nlohmann::json::parse(packet.data());

		Version version(jsonObject[VERSION].get<std::string>());
		LOG_INFO("Checking for updates, client version: {}", version.getAsString());

		CheckResult checkResult;
		std::string newVersionString;
		nlohmann::json responseJson;

		if (version != VERSION_LOST) {
			bool hasMajorUpdate = false;
			Version latestVersion = version;

			for (const auto& entry : std::filesystem::directory_iterator(m_versionsDirectory)) {
				if (entry.is_directory()) {
					std::filesystem::path versionJsonPath = entry.path() / "version.json";

					if (std::filesystem::exists(versionJsonPath) &&
						std::filesystem::is_regular_file(versionJsonPath)) {
						std::ifstream file(versionJsonPath);
						nlohmann::json versionJson = nlohmann::json::parse(file);

						Version currentVersion(versionJson[VERSION].get<std::string>());
						std::string updateType = versionJson[UPDATE_TYPE].get<std::string>();

						if (currentVersion > version) {
							if (updateType == MAJOR_UPDATE) {
								hasMajorUpdate = true;
							}

							if (currentVersion > latestVersion) {
								latestVersion = currentVersion;
							}
						}
					}
				}
			}

			if (latestVersion == version) {
				checkResult = CheckResult::UPDATE_NOT_NEEDED;
				LOG_INFO("Client is up to date (version: {})", version.getAsString());
			}
			else if (hasMajorUpdate) {
				checkResult = CheckResult::REQUIRED_UPDATE;
				newVersionString = latestVersion.getAsString();
				LOG_INFO("Major update available for client (current: {}, latest: {})", version.getAsString(), latestVersion.getAsString());
			}
			else {
				checkResult = CheckResult::POSSIBLE_UPDATE;
				newVersionString = latestVersion.getAsString();
				LOG_INFO("Minor update available for client (current: {}, latest: {})", version.getAsString(), latestVersion.getAsString());
			}
		}
		else {
			checkResult = CheckResult::REQUIRED_UPDATE;
			auto [path, ver] = findLatestVersion();
			newVersionString = ver;
			LOG_WARN("Client has invalid version, requiring update");
		}

		responseJson[UPDATE_CHECK_RESULT] = static_cast<int>(checkResult);
		if (!newVersionString.empty()) {
			responseJson[VERSION] = newVersionString;
		}

		Packet packetResponse;
		packetResponse.setType(static_cast<int>(PacketType::UPDATE_RESULT));
		packetResponse.setData(responseJson.dump());

		connection->sendPacket(packetResponse);
	}

	std::vector<std::pair<std::filesystem::path, std::string>> 
		ServerUpdater::parseClientFiles(const nlohmann::json& jsonObject)
	{
		std::vector<std::pair<std::filesystem::path, std::string>> filePathsWithHashes;

		if (!jsonObject.contains(FILES) || !jsonObject[FILES].is_array()) {
			LOG_ERROR("Missing or invalid FILES field in packet");
			return filePathsWithHashes;
		}

		for (const auto& fileInfo : jsonObject[FILES]) {
			if (fileInfo.contains(RELATIVE_FILE_PATH) && fileInfo.contains(FILE_HASH)) {
				std::string relativePath = fileInfo[RELATIVE_FILE_PATH].get<std::string>();
				std::string fileHash = fileInfo[FILE_HASH].get<std::string>();
				filePathsWithHashes.emplace_back(relativePath, fileHash);
			}
			else {
				LOG_ERROR("Incomplete file information in packet - missing RelativeFilePath or FileHash");
			}
		}

		return filePathsWithHashes;
	}

	std::filesystem::path ServerUpdater::getOSSpecificPath(const std::filesystem::path& versionPath, OperationSystemType osType)
	{
		std::filesystem::path osSpecificPath;

		switch (osType) {
		case OperationSystemType::WINDOWS:
			osSpecificPath = versionPath / "Windows";
			LOG_DEBUG("Client OS: Windows");
			break;
		case OperationSystemType::LINUX:
			osSpecificPath = versionPath / "Linux";
			LOG_DEBUG("Client OS: Linux");
			break;
		case OperationSystemType::MAC:
			osSpecificPath = versionPath / "Mac";
			LOG_DEBUG("Client OS: Mac");
			break;
		default:
			LOG_ERROR("Unknown operating system type");
			break;
		}

		return osSpecificPath;
	}

	std::unordered_map<std::filesystem::path, std::string> 
		ServerUpdater::scanServerFiles(const std::filesystem::path& osSpecificPath)
	{
		std::unordered_map<std::filesystem::path, std::string> serverFiles;

		if (!std::filesystem::exists(osSpecificPath)) {
			LOG_ERROR("OS-specific folder does not exist: {}", osSpecificPath.string());
			return serverFiles;
		}

		for (const auto& entry : std::filesystem::recursive_directory_iterator(osSpecificPath)) {
			if (entry.is_regular_file()) {
				std::filesystem::path relativePath = std::filesystem::relative(entry.path(), osSpecificPath);
				std::string fileHash = utilities::calculateFileHash(entry.path());
				serverFiles[relativePath] = fileHash;
			}
		}

		LOG_DEBUG("Scanned {} files on server", serverFiles.size());
		return serverFiles;
	}

	std::pair<std::vector<std::filesystem::path>, std::vector<std::filesystem::path>> 
		ServerUpdater::determineFilesToSync(
			const std::vector<std::pair<std::filesystem::path, std::string>>& clientFiles,
			const std::unordered_map<std::filesystem::path, std::string>& serverFiles)
	{
		std::vector<std::filesystem::path> filesToDelete;
		std::vector<std::filesystem::path> filesToSend;
		std::unordered_set<std::filesystem::path> clientFilePaths;

		for (const auto& [clientRelativePath, clientHash] : clientFiles) {
			clientFilePaths.insert(clientRelativePath);

			auto serverFileIt = serverFiles.find(clientRelativePath);
			
			if (serverFileIt == serverFiles.end()) {
				filesToDelete.push_back(clientRelativePath);
				LOG_DEBUG("File marked for deletion (not on server): {}", clientRelativePath.string());
			}
			else {
				const std::string& serverHash = serverFileIt->second;
				
				if (serverHash != clientHash) {
					filesToSend.push_back(clientRelativePath);
					LOG_DEBUG("File marked for update (hash mismatch): {} (client: {}, server: {})", 
						clientRelativePath.string(), clientHash.substr(0, 8), serverHash.substr(0, 8));
				}
				else {
					LOG_DEBUG("File is up to date (hash matches): {}", clientRelativePath.string());
				}
			}
		}

		for (const auto& [serverRelativePath, serverHash] : serverFiles) {
			if (clientFilePaths.find(serverRelativePath) == clientFilePaths.end()) {
				filesToSend.push_back(serverRelativePath);
				LOG_DEBUG("File marked for download (new file on server): {}", serverRelativePath.string());
			}
		}

		LOG_INFO("File sync analysis: {} files to download, {} files to delete", 
			filesToSend.size(), filesToDelete.size());

		return std::make_pair(filesToSend, filesToDelete);
	}

	nlohmann::json ServerUpdater::createUpdateMetadata(
		const std::vector<std::filesystem::path>& filesToSend,
		const std::vector<std::filesystem::path>& filesToDelete,
		const std::unordered_map<std::filesystem::path, std::string>& serverFiles,
		const std::filesystem::path& osSpecificPath,
		const std::string& version)
	{
		nlohmann::json responseJson;

		nlohmann::json filesToDownloadArray = nlohmann::json::array();
		for (const auto& filePath : filesToSend) {
			auto fullPath = osSpecificPath / filePath;
			if (std::filesystem::exists(fullPath)) {
				uint64_t fileSize = std::filesystem::file_size(fullPath);
				std::string fileHash = serverFiles.at(filePath);

				nlohmann::json fileEntry;
				fileEntry[RELATIVE_FILE_PATH] = filePath.string();
				fileEntry[FILE_SIZE] = fileSize;
				fileEntry[FILE_HASH] = fileHash;

				filesToDownloadArray.push_back(fileEntry);
			}
			else {
				LOG_WARN("File marked for sending does not exist: {}", fullPath.string());
			}
		}

		nlohmann::json filesToDeleteArray = nlohmann::json::array();
		for (const auto& filePath : filesToDelete) {
			filesToDeleteArray.push_back(filePath.string());
		}

		responseJson[FILES_TO_DOWNLOAD] = filesToDownloadArray;
		responseJson[FILES_TO_DELETE] = filesToDeleteArray;
		responseJson[VERSION] = version;

		return responseJson;
	}

	void ServerUpdater::sendUpdateMetadata(ConnectionPtr connection, const nlohmann::json& metadata)
	{
		Packet metadataPacket;
		metadataPacket.setType(static_cast<int>(PacketType::UPDATE_METADATA));
		metadataPacket.setData(metadata.dump());

		connection->sendPacket(metadataPacket);
	}

	void ServerUpdater::sendUpdateFiles(ConnectionPtr connection, 
										 const std::vector<std::filesystem::path>& filesToSend,
										 const std::filesystem::path& osSpecificPath)
	{
		for (const auto& relativePath : filesToSend) {
			auto fullPath = osSpecificPath / relativePath;
			if (std::filesystem::exists(fullPath)) {
				connection->sendFile(fullPath);
				LOG_DEBUG("Sending file: {}", relativePath.string());
			}
			else {
				LOG_WARN("Attempted to send non-existent file: {}", fullPath.string());
			}
		}
	}

	void ServerUpdater::onUpdateAccepted(ConnectionPtr connection, Packet&& packet)
	{
		LOG_INFO("Client accepted update, preparing files");
		
		nlohmann::json jsonObject;
		try {
			jsonObject = nlohmann::json::parse(packet.data());
		}
		catch (const std::exception& e) {
			LOG_ERROR("Failed to parse packet JSON: {}", e.what());
			return;
		}

		if (!jsonObject.contains(OPERATION_SYSTEM)) {
			LOG_ERROR("Missing OPERATION_SYSTEM field in packet");
			return;
		}
		OperationSystemType osType = static_cast<OperationSystemType>(jsonObject[OPERATION_SYSTEM].get<int>());

		std::vector<std::pair<std::filesystem::path, std::string>> clientFiles = parseClientFiles(jsonObject);
		if (clientFiles.empty() && jsonObject.contains(FILES)) {
			LOG_WARN("No valid client files found in packet");
		}

		auto [latestVersionPath, latestVersion] = findLatestVersion();
		if (latestVersion.empty()) {
			LOG_ERROR("Failed to find latest version");
			return;
		}

		std::filesystem::path osSpecificPath = getOSSpecificPath(latestVersionPath, osType);
		if (osSpecificPath.empty()) {
			return;
		}

		if (!std::filesystem::exists(osSpecificPath)) {
			LOG_ERROR("OS-specific folder does not exist: {}", osSpecificPath.string());
			return;
		}

		std::unordered_map<std::filesystem::path, std::string> serverFiles = scanServerFiles(osSpecificPath);
		if (serverFiles.empty()) {
			LOG_WARN("No files found on server in: {}", osSpecificPath.string());
		}

		auto [filesToSend, filesToDelete] = determineFilesToSync(clientFiles, serverFiles);

		nlohmann::json metadata = createUpdateMetadata(filesToSend, filesToDelete, serverFiles, osSpecificPath, latestVersion);

		LOG_INFO("Sending update: {} files to download, {} files to delete", filesToSend.size(), filesToDelete.size());
		LOG_DEBUG("Update version: {}", latestVersion);

		sendUpdateMetadata(connection, metadata);

		sendUpdateFiles(connection, filesToSend, osSpecificPath);
	}
}
