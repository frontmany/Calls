#pragma once
#include <functional>
#include <string>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <filesystem>

#include "checkResult.h"
#include "operationSystemType.h"
#include "networkController.h"
#include "jsonTypes.h"

#include "json.hpp"

namespace updater {

class ClientUpdater {
public:
	ClientUpdater(
		std::function<void(CheckResult)>&& onCheckResult,
		std::function<void()>&& onUpdateLoaded,
		std::function<void()>&& onError
	);
	void connect(const std::string& host, const std::string& port);
	void disconnect();
	void checkUpdates(const std::string& currentVersionNumber, OperationSystemType type);
	void startUpdate();
	void declineUpdate();

private:
	std::vector<std::pair<std::filesystem::path, std::string>> getFilePathsWithHashes();
	bool checkRemoveJsonExists();
	void processRemoveJson();

private:
	NetworkController m_networkController;
	std::function<void(CheckResult)> m_onCheckResult;
	std::function<void()> m_onUpdateLoaded;
	std::function<void()> m_onError;
};

}