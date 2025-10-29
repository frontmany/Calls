#pragma once
#include <functional>
#include <string>
#include <fstream>
#include <sstream>
#include <atomic>
#include <iomanip>
#include <filesystem>

#include "checkResult.h"
#include "safeDeque.h"
#include "operationSystemType.h"
#include "networkController.h"
#include "jsonTypes.h"

#include "json.hpp"

namespace updater {

class ClientUpdater {
public:
	enum class State {
		DISCONNECTED,
		AWAITING_SERVER_RESPONSE,
		AWAITING_UPDATES_CHECK,
		AWAITING_START_UPDATE
	};

	ClientUpdater(
		std::function<void(CheckResult)>&& onCheckResult,
		std::function<void(double)>&& onLoadingProgress,
		std::function<void()>&& onUpdateLoaded,
		std::function<void()>&& onError
	);
	~ClientUpdater();
	bool connect(const std::string& host, const std::string& port);
	void disconnect();
	bool checkUpdates(const std::string& currentVersionNumber);
	bool startUpdate(OperationSystemType type);
	State getState();

private:
	void processQueue();
	std::vector<std::pair<std::filesystem::path, std::string>> getFilePathsWithHashes();

private:
	SafeDeque<std::function<void()>> m_queue;
	std::thread m_queueProcessingThread;

	std::atomic_bool m_running = false;
	std::atomic<State> m_state = State::DISCONNECTED;
	NetworkController m_networkController;
	std::function<void(CheckResult)> m_onCheckResult;
	std::function<void(double)> m_onLoadingProgress;
	std::function<void()> m_onUpdateLoaded;
	std::function<void()> m_onError;
};

}