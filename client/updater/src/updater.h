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

#include "utilities/utilities.h"
#include "eventListener.h"
#include "updateCheckResult.h"
#include "operationSystemType.h"
#include "network/networkController.h"
#include "jsonType.h"
#include "packetType.h"
#include "network/packet.h"
#include "network/fileReceiver.h"

#include "json.hpp"

namespace updater
{
	class Client {
	public:
		Client();
		~Client();

		void init(std::shared_ptr<EventListener> eventListener,
			const std::string& tempDirectory,
			const std::string& deletionListFileName,
			const std::unordered_set<std::string>& ignoredFiles,
			const std::unordered_set<std::string>& excludedDirectories);

		void start( const std::string& host, const std::string& port);
		void stop();
		void checkUpdates(const std::string& currentVersionNumber);
		bool startUpdate(OperationSystemType type);
		bool isConnected();

	private:
		std::string normalizePath(const std::filesystem::path& path);
		std::vector<std::pair<std::filesystem::path, std::string>> getFilePathsWithHashes();
		void deleteTempDirectory();
		std::vector<network::FileMetadata> parseMetadata(network::Packet& packet);
		void reconnectLoop();

	private:
		std::future<void> m_checkUpdatesFuture;
		network::NetworkController m_networkController;
		std::shared_ptr<EventListener> m_eventListener;
		std::string m_serverHost;
		std::string m_serverPort;
		std::string m_tempDirectory;
		std::string m_deletionListFileName;
		std::unordered_set<std::string> m_ignoredFiles;
		std::unordered_set<std::string> m_excludedDirectories;
		std::atomic_bool m_reconnecting = false;
		std::atomic_bool m_stopReconnectThread = false;
		std::thread m_reconnectThread;
		std::mutex m_reconnectMutex;
		std::condition_variable m_reconnectCondition;
	};
}
