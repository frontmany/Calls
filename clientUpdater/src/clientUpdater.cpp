#include "clientUpdater.h"
#include "utility.h"
#include "packetTypes.h"

namespace updater {

ClientUpdater::ClientUpdater(
	std::function<void(CheckResult)>&& onCheckResult,
	std::function<void()>&& onUpdateLoaded,
	std::function<void()>&& onError)
	: m_onCheckResult(std::move(onCheckResult)),
	m_onUpdateLoaded(std::move(onUpdateLoaded)),
	m_onError(std::move(onError)),
	m_networkController(
		[this](CheckResult result) {m_queue.push_back([this, result]() {m_state = State::AWAITING_START_UPDATE; m_onCheckResult(result); }); },
		[this]() {m_queue.push_back([this]() {m_state = State::AWAITING_UPDATES_CHECK; m_onUpdateLoaded(); }); },
		[this]() {m_queue.push_back([this]() {m_state = State::AWAITING_UPDATES_CHECK;});  },
		[this]() {m_queue.push_back([this]() {m_state = State::DISCONNECTED; m_networkController.disconnect(); m_running = false; m_onError(); }); })
{
}

ClientUpdater::~ClientUpdater() {
	m_running = false;
	if (m_queueProcessingThread.joinable()) {
		m_queueProcessingThread.join();
	}
}

bool ClientUpdater::connect(const std::string& host, const std::string& port) {
	if (m_state != State::DISCONNECTED || m_running) return false;

	m_networkController.connect(host, port);

	m_running = true;
	m_queueProcessingThread = std::thread([this]() {processQueue(); });

	return true;
}

void ClientUpdater::disconnect() {
	m_networkController.disconnect();
	m_state = State::DISCONNECTED;

	m_running = false;
	if (m_queueProcessingThread.joinable()) {
		m_queueProcessingThread.join();
	}
}

void ClientUpdater::processQueue() {
	while (m_running) {
		if (m_queue.size() != 0) {
			auto callback = m_queue.pop_front();
			callback();
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}

bool ClientUpdater::checkUpdates(const std::string& currentVersionNumber) {
	if (m_state != State::AWAITING_UPDATES_CHECK) return false;

	nlohmann::json jsonObject;
	jsonObject["version"] = currentVersionNumber;
	Packet packet(static_cast<int>(PacketType::CHECK_UPDATES), jsonObject.dump());

	m_networkController.sendPacket(packet);

	m_state = State::AWAITING_SERVER_RESPONSE;
	return true;
}

bool ClientUpdater::startUpdate(OperationSystemType type) {
	if (m_state != State::AWAITING_START_UPDATE) return false;

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

	Packet packet(static_cast<int>(PacketType::UPDATE_ACCEPT), jsonString);
	m_networkController.sendPacket(packet);

	m_state = State::AWAITING_SERVER_RESPONSE;
	return true;
}

ClientUpdater::State ClientUpdater::getState() {
	return m_state;
}

std::vector<std::pair<std::filesystem::path, std::string>> ClientUpdater::getFilePathsWithHashes() {
	std::vector<std::pair<std::filesystem::path, std::string>> result;

	try {
		std::filesystem::path currentPath = std::filesystem::current_path();
		std::filesystem::path tempDir = "update_temp";

		for (const auto& entry : std::filesystem::recursive_directory_iterator(currentPath)) {
			if (entry.is_regular_file()) {
				std::filesystem::path relativePath = std::filesystem::relative(entry.path(), currentPath);

				if (entry.path().filename() == "config.json" || entry.path().filename() == "config" || entry.path().filename() == "updater.exe" || entry.path().filename() == "updater") {
					continue;
				}

				if (relativePath.string().find("update_temp") == 0 ||
					relativePath.string().find(".vs") == 0) {
					continue;
				}

				std::string hash = calculateFileHash(entry.path());
				result.emplace_back(relativePath, std::move(hash));
			}
		}
	}
	catch (const std::filesystem::filesystem_error& e) {
		m_onError();
	}

	return result;
}

}