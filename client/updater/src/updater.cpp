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
			[this](CheckResult status, const std::string& newVersion) {
				if (m_eventListener) {
					m_eventListener->onUpdateCheckResult(status, newVersion);
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

				std::this_thread::sleep_for(reconnectInterval);

				lock.lock();
			}
			else {
				m_reconnectCondition.wait(lock);
				if (!m_stopReconnectThread) {
					std::this_thread::sleep_for(reconnectInterval);
				}
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

		auto pathsWithHashes = getFilePathsWithHashes(nullptr);

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

	void Client::startUpdateAsync(OperationSystemType type, std::shared_ptr<Client> self)
	{
		if (!isConnected() || !self) return;

		m_loadingUpdate = true;

		std::shared_ptr<EventListener> listener = m_eventListener;
		std::thread([type, self = std::move(self), listener = std::move(listener)]() {
			ManifestProgressCallback progressCb;
			if (listener) {
				progressCb = [listener](std::size_t current, std::size_t total, const std::string& path) {
					listener->onManifestProgress(current, total, path);
				};
			}
			auto pathsWithHashes = self->getFilePathsWithHashes(progressCb);
			self->sendUpdateAcceptPacket(type, pathsWithHashes);
		}).detach();
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

	namespace
	{
		struct PathFilterContext {
			std::filesystem::path basePath;
			std::filesystem::path tempDirectoryPath;
			const std::unordered_set<std::string>& ignoredFiles;
			const std::unordered_set<std::string>& ignoredDirectories;
		};

		bool shouldIncludeFile(const std::filesystem::path& absolutePath, const std::filesystem::path& relativePath, const PathFilterContext& ctx) {
			if (!ctx.tempDirectoryPath.empty()) {
				std::string tempDirStr = ctx.tempDirectoryPath.generic_string();
				if (!tempDirStr.empty()) {
					if (tempDirStr.back() != '/') tempDirStr.push_back('/');
					std::string entryPathStr = absolutePath.generic_string();
					if (entryPathStr.rfind(tempDirStr, 0) == 0) return false;
				}
			}
			std::string filename = absolutePath.filename().string();
			if (ctx.ignoredFiles.find(filename) != ctx.ignoredFiles.end()) return false;
			std::string relativePathStr = relativePath.generic_string();
			for (const auto& excludedDir : ctx.ignoredDirectories) {
				if (relativePathStr.find(excludedDir) == 0) return false;
			}
			return true;
		}
	}

	void Client::sendUpdateAcceptPacket(OperationSystemType type, const std::vector<std::pair<std::filesystem::path, std::string>>& pathsWithHashes)
	{
		nlohmann::json jsonObject;
		jsonObject[OPERATION_SYSTEM] = static_cast<int>(type);
		nlohmann::json filesArray = nlohmann::json::array();
		for (const auto& [path, hash] : pathsWithHashes) {
			nlohmann::json fileInfo;
			fileInfo[RELATIVE_FILE_PATH] = path.string();
			fileInfo[FILE_HASH] = hash;
			filesArray.push_back(fileInfo);
		}
		jsonObject[FILES] = filesArray;
		network::Packet packet(static_cast<uint32_t>(PacketType::UPDATE_ACCEPT), jsonObject.dump());
		m_networkController.sendPacket(packet);
	}

	std::vector<std::pair<std::filesystem::path, std::string>> Client::getFilePathsWithHashes(ManifestProgressCallback progressCallback) {
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

			PathFilterContext ctx{ basePath, tempDirectoryPath, m_ignoredFiles, m_ignoredDirectories };

			// First pass: collect all paths to hash (so we can report total and progress)
			std::vector<std::pair<std::filesystem::path, std::filesystem::path>> pathsToHash; // absolute, relative
			for (const auto& entry : std::filesystem::recursive_directory_iterator(basePath)) {
				if (entry.is_regular_file()) {
					std::filesystem::path relativePath = std::filesystem::relative(entry.path(), basePath);
					if (!shouldIncludeFile(entry.path(), relativePath, ctx)) continue;
					pathsToHash.emplace_back(entry.path(), std::move(relativePath));
				}
			}

			const std::size_t total = pathsToHash.size();
			for (std::size_t i = 0; i < total; ++i) {
				const auto& [absolutePath, relativePath] = pathsToHash[i];
				if (progressCallback) {
					progressCallback(i, total, absolutePath.generic_string());
				}
				std::string hash = utilities::calculateFileHash(absolutePath);
				std::string normalizedPathStr = normalizePath(relativePath);
				result.emplace_back(std::filesystem::path(normalizedPathStr), std::move(hash));
			}
		}
		catch (const std::filesystem::filesystem_error&) {
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
