#include "updater.h"
#include "checkResult.h"

#include <thread>
#include <chrono>
#include <cmath>
#include <iomanip>

namespace updater
{
	Client::Client()
		: m_networkController(
			[this](CheckResult status) {
				if (m_eventListener) {
					m_eventListener->onUpdateCheckResult(status);
				}
			},
			[this](double progress) {
				if (m_eventListener) {
					m_eventListener->onLoadingProgress(progress);
				}
			},
			[this](bool emptyUpdate) {
				m_loadingUpdate = false;
				if (m_eventListener) {
					m_eventListener->onUpdateLoaded(emptyUpdate);
				}
			},
			[this]() {
				if (!m_reconnecting.exchange(true)) {
					if (m_eventListener) {
						m_eventListener->onNetworkError();
					}
					m_reconnectCondition.notify_one();
				}
			},
			[this](network::Packet& packet) {
				return parseMetadata(packet);
			},
			[this]() {
				m_reconnecting = false;
				m_loadingUpdate = false;
				if (m_eventListener) {
					m_eventListener->onConnected();
				}
			})
	{
		m_reconnectThread = std::thread([this]() {
			reconnectLoop();
		});
	}

	Client::~Client()
	{
		stop();
		if (m_reconnectThread.joinable()) {
			m_stopReconnectThread = true;
			m_reconnectCondition.notify_one();
			m_reconnectThread.join();
		}
	}

	void Client::init(std::shared_ptr<EventListener> eventListener,
		const std::string& appDirectory,
		const std::string& tempDirectory,
		const std::string& deletionListFileName,
		const std::unordered_set<std::string>& ignoredFiles,
		const std::unordered_set<std::string>& ignoredDirectories)
	{
		m_eventListener = eventListener;
		m_appDirectory = appDirectory;
		m_tempDirectory = tempDirectory;
		m_deletionListFileName = deletionListFileName;
		m_ignoredFiles = ignoredFiles;
		m_ignoredDirectories = ignoredDirectories;
	}

	void Client::start(const std::string& host, const std::string& port)
	{
		m_serverHost = host;
		m_serverPort = port;
		m_reconnecting = false;
		m_networkController.connect(host, port);
	}

	void Client::stop()
	{
		m_reconnecting = false;
		m_loadingUpdate = false;
		m_networkController.disconnect();
		m_reconnectCondition.notify_one();
	}

	void Client::reconnectLoop()
	{
		constexpr auto reconnectInterval = std::chrono::seconds(2);
		std::unique_lock<std::mutex> lock(m_reconnectMutex);

		while (!m_stopReconnectThread) {
			if (m_reconnecting) {
				lock.unlock();

				m_networkController.connect(m_serverHost, m_serverPort);

				lock.lock();
				
				if (m_reconnecting && !m_stopReconnectThread) {
					m_reconnectCondition.wait_for(lock, reconnectInterval);
				}
			}
			else {
				m_reconnectCondition.wait(lock);
			}
		}
	}

	bool Client::checkUpdates(const std::string& currentVersionNumber)
	{
		if (!isConnected()) return false;

		nlohmann::json jsonObject;
		jsonObject[VERSION] = currentVersionNumber;
		network::Packet packet(static_cast<uint32_t>(PacketType::UPDATE_CHECK), jsonObject.dump());
		m_networkController.sendPacket(packet);

		return true;
	}

	bool Client::startUpdate(OperationSystemType type)
	{
		if (!isConnected()) return false;

		m_loadingUpdate = true;

		nlohmann::json jsonObject;
		jsonObject[OPERATION_SYSTEM] = static_cast<int>(type);

		auto pathsWithHashes = getFilePathsWithHashes();

		nlohmann::json filesArray = nlohmann::json::array();
		for (const auto& [path, hash] : pathsWithHashes) {
			nlohmann::json fileInfo;
			fileInfo[RELATIVE_FILE_PATH] = path.string();
			fileInfo[FILE_HASH] = hash;

			filesArray.push_back(fileInfo);
		}

		jsonObject[FILES] = filesArray;

		std::string jsonString = jsonObject.dump();

		network::Packet packet(static_cast<uint32_t>(PacketType::UPDATE_ACCEPT), jsonString);
		m_networkController.sendPacket(packet);

		return true;
	}

	bool Client::isConnected()
	{
		return m_networkController.isConnected();
	}

	bool Client::isLoadingUpdate()
	{
		return m_loadingUpdate;
	}

	std::string Client::normalizePath(const std::filesystem::path& path) {
		std::string normalized = path.generic_string();

		if (normalized.find("./") == 0) {
			normalized = normalized.substr(2);
		}

		return normalized;
	}

	std::vector<std::pair<std::filesystem::path, std::string>> Client::getFilePathsWithHashes() {
		std::vector<std::pair<std::filesystem::path, std::string>> result;

		try {
			std::filesystem::path basePath = m_appDirectory.empty()
				? std::filesystem::current_path()
				: std::filesystem::path(m_appDirectory);
			basePath = std::filesystem::absolute(basePath);

			std::filesystem::path tempDirectoryPath;
			if (!m_tempDirectory.empty()) {
				tempDirectoryPath = std::filesystem::absolute(std::filesystem::path(m_tempDirectory));
			}

			for (const auto& entry : std::filesystem::recursive_directory_iterator(basePath)) {
				if (entry.is_regular_file()) {
					if (!tempDirectoryPath.empty()) {
						std::string tempDirStr = tempDirectoryPath.generic_string();
						if (!tempDirStr.empty()) {
							if (tempDirStr.back() != '/') {
								tempDirStr.push_back('/');
							}
							std::string entryPathStr = std::filesystem::absolute(entry.path()).generic_string();
							if (entryPathStr.rfind(tempDirStr, 0) == 0) {
								continue;
							}
						}
					}

					std::filesystem::path relativePath = std::filesystem::relative(entry.path(), basePath);
					std::string filename = entry.path().filename().string();

					if (m_ignoredFiles.find(filename) != m_ignoredFiles.end()) {
						continue;
					}

					std::string relativePathStr = relativePath.generic_string();

					bool isExcluded = false;
					for (const auto& excludedDir : m_ignoredDirectories) {
						if (relativePathStr.find(excludedDir) == 0) {
							isExcluded = true;
							break;
						}
					}
					if (isExcluded) {
						continue;
					}

					std::string hash = utilities::calculateFileHash(entry.path());
					std::string normalizedPathStr = normalizePath(relativePath);
					std::filesystem::path normalizedPath(normalizedPathStr);

					result.emplace_back(normalizedPath, std::move(hash));
				}
			}
		}
		catch (const std::filesystem::filesystem_error& e) {
			if (m_eventListener) {
				m_eventListener->onNetworkError();
			}
		}

		return result;
	}

	void Client::deleteTempDirectory()
	{
		std::filesystem::path tempDirectory = m_tempDirectory;
		std::filesystem::remove_all(tempDirectory);
		std::filesystem::remove(tempDirectory);
	}

	std::vector<network::FileMetadata> Client::parseMetadata(network::Packet& packet)
	{
		deleteTempDirectory();

		nlohmann::json jsonObject = nlohmann::json::parse(packet.data());

		std::filesystem::path tempDirectory = m_tempDirectory;
		std::vector<network::FileMetadata> files;

		try {
			if (!std::filesystem::exists(tempDirectory)) {
				std::filesystem::create_directories(tempDirectory);
			}

			if (jsonObject.contains(FILES_TO_DOWNLOAD)) {
				for (const auto& fileEntry : jsonObject[FILES_TO_DOWNLOAD]) {
					if (fileEntry.contains(RELATIVE_FILE_PATH) && fileEntry.contains(FILE_SIZE) && fileEntry.contains(FILE_HASH)) {
						std::string relativeFilePath = fileEntry[RELATIVE_FILE_PATH].get<std::string>();
						std::string fileHash = fileEntry[FILE_HASH].get<std::string>();
						uint64_t fileSize = fileEntry[FILE_SIZE].get<uint64_t>();

						int chunksCount = static_cast<int>(std::ceil(static_cast<double>(fileSize) / network::FileReceiver::c_chunkSize));
						uint64_t lastChunkSize = fileSize % network::FileReceiver::c_chunkSize;

						if (lastChunkSize == 0) {
							lastChunkSize = network::FileReceiver::c_chunkSize;
						}

						network::FileMetadata fileInfo;
						fileInfo.fileHash = fileHash;
						fileInfo.expectedChunksCount = chunksCount;
						fileInfo.relativeFilePath = tempDirectory / relativeFilePath;
						fileInfo.lastChunkSize = lastChunkSize;
						fileInfo.fileSize = fileSize;

						files.push_back(std::move(fileInfo));
					}
				}
			}

			if (jsonObject.contains(FILES_TO_DELETE)) {
				nlohmann::json removeJson;
				nlohmann::json filesArray = nlohmann::json::array();

				for (const auto& filePath : jsonObject[FILES_TO_DELETE]) {
					if (filePath.is_string()) {
						filesArray.push_back(filePath.get<std::string>());
					}
				}

				removeJson[FILES] = filesArray;

				std::filesystem::path removeJsonPath = tempDirectory / m_deletionListFileName;
				std::ofstream file(removeJsonPath);
				if (file) {
					file << std::setw(4) << removeJson << std::endl;
					file.close();
				}
			}
		}
		catch (const nlohmann::json::exception& e) {
			if (m_eventListener) {
				m_eventListener->onNetworkError();
			}
		}

		return files;
	}
}
