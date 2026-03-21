#pragma once
#include <functional>
#include <condition_variable>
#include <mutex>
#include <string>
#include <memory>
#include <fstream>
#include <sstream>
#include <atomic>
#include <iomanip>
#include <filesystem>
#include <future>
#include <unordered_set>
#include <cstddef>

#include "utilities/utilities.h"
#include "eventListener.h"
#include "checkResult.h"
#include "operationSystemType.h"
#include "network/networkController.h"
#include "jsonType.h"
#include "packetType.h"
#include "network/packet.h"
#include "network/fileReceiver.h"

#include <nlohmann/json.hpp>

namespace updater
{
	/// Optional progress callback for manifest preparation: (filesProcessed, totalFiles, currentFilePath). Called from worker thread.
	using ManifestProgressCallback = std::function<void(std::size_t, std::size_t, const std::string&)>;

	class Client {
	public:
		Client();
		~Client();

		void init(std::shared_ptr<EventListener> eventListener,
			const std::string& appDirectory,
			const std::string& tempDirectory,
			const std::string& deletionListFileName,
			const std::unordered_set<std::string>& ignoredFiles,
			const std::unordered_set<std::string>& ignoredDirectories);

		void start( const std::string& host, const std::string& port);
		void stop();
		bool checkUpdates(const std::string& currentVersionNumber);
		/// Synchronous: hashes all files on calling thread (blocks). Prefer startUpdateAsync for UI.
		bool startUpdate(OperationSystemType type);
		/// Non-blocking: runs manifest preparation in a background thread, then sends UPDATE_ACCEPT. Progress via EventListener::onManifestProgress. Caller must pass shared_ptr to this so the thread keeps Client alive.
		void startUpdateAsync(OperationSystemType type, std::shared_ptr<Client> self);
		bool isConnected();
		bool isLoadingUpdate();

	private:
		std::string normalizePath(const std::filesystem::path& path);
		/// Collects paths (same rules as full scan), then hashes each; optionally calls progressCallback(filesProcessed, totalFiles, currentPath) from calling thread.
		std::vector<std::pair<std::filesystem::path, std::string>> getFilePathsWithHashes(ManifestProgressCallback progressCallback = nullptr);
		void sendUpdateAcceptPacket(OperationSystemType type, const std::vector<std::pair<std::filesystem::path, std::string>>& pathsWithHashes);
		void deleteTempDirectory();
		std::vector<network::FileMetadata> parseMetadata(network::Packet& packet);
		void reconnectLoop();

	private:
		network::NetworkController m_networkController;
		std::shared_ptr<EventListener> m_eventListener;

		std::string m_serverHost;
		std::string m_serverPort;
		std::string m_appDirectory;
		std::string m_tempDirectory;
		std::string m_deletionListFileName;
		std::unordered_set<std::string> m_ignoredFiles;
		std::unordered_set<std::string> m_ignoredDirectories;

		std::atomic_bool m_reconnecting = false;
		std::atomic_bool m_stopReconnectThread = false;
		std::atomic_bool m_loadingUpdate = false;
		std::thread m_reconnectThread;
		std::mutex m_reconnectMutex;
		std::condition_variable m_reconnectCondition;
	};
}
