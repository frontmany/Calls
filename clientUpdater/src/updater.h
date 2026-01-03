#pragma once
#include <functional>
#include <condition_variable>
#include <string>
#include <memory>
#include <fstream>
#include <sstream>
#include <atomic>
#include <iomanip>
#include <filesystem>
#include <future>

#include "utility.h"
#include "eventListener.h"
#include "updatesCheckResult.h"
#include "safeDeque.h"
#include "operationSystemType.h"
#include "networkController.h"
#include "jsonTypes.h"
#include "packetTypes.h"
#include "state.h"

#include "json.hpp"

namespace callifornia {

class Updater {
public:
	Updater();
	~Updater();

	void init(std::shared_ptr<EventListener> eventListener);
	bool connect(const std::string& host, const std::string& port);
	void disconnect();
	void checkUpdates(const std::string& currentVersionNumber);
	bool startUpdate(OperationSystemType type);
	bool isConnected();
	bool isAwaitingServerResponse();
	bool isAwaitingCheckUpdatesFunctionCall();
	bool isAwaitingStartUpdateFunctionCall();
	const std::string& getServerHost();
	const std::string& getServerPort();

private:
	void processQueue();
	std::string normalizePath(const std::filesystem::path& path);
	std::vector<std::pair<std::filesystem::path, std::string>> getFilePathsWithHashes();

private:
	SafeDeque<std::function<void()>> m_queue;
	std::thread m_queueProcessingThread;

	std::atomic_bool m_running = false;
	std::atomic_bool m_processingProgress = false;
	std::atomic<State> m_state = State::DISCONNECTED;
	std::future<void> m_checkUpdatesFuture;

	NetworkController m_networkController;
	std::shared_ptr<EventListener> m_eventListener;

	std::string m_serverHost;
	std::string m_serverPort;
		};

	}
}
