#include "clientUpdater.h"
#include "logger.h"

namespace updater {

ClientUpdater& ClientUpdater::get() {
	static ClientUpdater s_instance;
	return s_instance;
}

ClientUpdater::ClientUpdater()
	: m_networkController(
		[this](UpdatesCheckResult result) {m_queue.push_back([this, result]() {m_state = State::AWAITING_START_UPDATE; m_callbacksHandler->onUpdatesCheckResult(result); }); },
		[this](double progress) {m_queue.push_back([this, progress]() {m_callbacksHandler->onLoadingProgress(progress); m_processingProgress = true; }); },
		[this](bool emptyUpdate) {m_queue.push_back([this, emptyUpdate]() {m_state = State::AWAITING_UPDATES_CHECK; m_callbacksHandler->onUpdateLoaded(emptyUpdate); m_processingProgress = false; }); },
		[this]() {m_queue.push_back([this]() {m_state = State::AWAITING_UPDATES_CHECK;});  },
		[this]() {m_queue.push_back([this]() {m_callbacksHandler->onError(); }); })
{
}

ClientUpdater::~ClientUpdater() {
	m_running = false;
	if (m_queueProcessingThread.joinable()) {
		m_queueProcessingThread.join();
	}
}

void ClientUpdater::init(std::unique_ptr<CallbacksInterface>&& callbacksHandler) {
	m_callbacksHandler = std::move(callbacksHandler);
}

bool ClientUpdater::connect(const std::string& host, const std::string& port) {
	m_serverHost = host;
	m_serverPort = port;

	if (m_state != State::DISCONNECTED || m_running) {
		LOG_WARN("Cannot connect - already connected or running");
		return false;
	}

	LOG_INFO("Connecting to update server {}:{}", host, port);
	m_networkController.connect(host, port);

	m_running = true;
	m_queueProcessingThread = std::thread([this]() {processQueue(); });

	return true;
}

void ClientUpdater::disconnect() {
	LOG_INFO("Disconnecting from update server");
	m_networkController.disconnect();
	m_state = State::DISCONNECTED;

	m_running = false;
	if (m_queueProcessingThread.joinable()) {
		m_queueProcessingThread.join();
	}

	m_queue.clear();
}

void ClientUpdater::processQueue() {
	while (m_running) {
		if (m_queue.size() != 0) {
			auto callback = m_queue.pop_front();
			callback();
		}
		
		if (!m_processingProgress) {
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
	}
}

bool ClientUpdater::checkUpdates(const std::string& currentVersionNumber) {
	auto startTime = std::chrono::steady_clock::now();
	auto timeout = std::chrono::seconds(5);

	while (std::chrono::steady_clock::now() - startTime < timeout) {
		if (m_state == State::AWAITING_UPDATES_CHECK) {
			break;
		}
	}

	if (m_state != State::AWAITING_UPDATES_CHECK) {
		LOG_WARN("Cannot check updates - invalid state");
		return false;
	}

	LOG_INFO("Checking for updates, current version: {}", currentVersionNumber);
	nlohmann::json jsonObject;
	jsonObject["version"] = currentVersionNumber;
	Packet packet(static_cast<int>(PacketType::CHECK_UPDATES), jsonObject.dump());

	m_networkController.sendPacket(packet);

	m_state = State::AWAITING_SERVER_RESPONSE;
	return true;
}

bool ClientUpdater::startUpdate(OperationSystemType type) {
	if (m_state != State::AWAITING_START_UPDATE) {
		LOG_WARN("Cannot start update - not ready");
		return false;
	}

	LOG_INFO("Starting update process");
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

bool ClientUpdater::isConnected() {
	return m_state != State::DISCONNECTED;
}

bool ClientUpdater::isAwaitingServerResponse() {
	return m_state == State::AWAITING_SERVER_RESPONSE;
}

bool ClientUpdater::isAwaitingCheckUpdatesFunctionCall() {
	return m_state == State::AWAITING_UPDATES_CHECK;
}

bool ClientUpdater::isAwaitingStartUpdateFunctionCall() {
	return m_state == State::AWAITING_START_UPDATE;
}

const::std::string& ClientUpdater::getServerHost() {
	return m_serverHost;
}

const::std::string& ClientUpdater::getServerPort() {
	return m_serverPort;
}

std::vector<std::pair<std::filesystem::path, std::string>> ClientUpdater::getFilePathsWithHashes() {
	std::vector<std::pair<std::filesystem::path, std::string>> result;

	try {
		std::filesystem::path currentPath = std::filesystem::current_path();
		std::filesystem::path tempDir = "update_temp";

		for (const auto& entry : std::filesystem::recursive_directory_iterator(currentPath)) {
			if (entry.is_regular_file()) {
				std::filesystem::path relativePath = std::filesystem::relative(entry.path(), currentPath);

				if (entry.path().filename() == "config.json" || entry.path().filename() == "config" || entry.path().filename() == "update_applier.exe" || entry.path().filename() == "update_applier") {
					continue;
				}

				if (relativePath.string().find("update_temp") == 0 || relativePath.string().find(".vs") == 0) {
					continue;
				}

				std::string hash = calculateFileHash(entry.path());
				result.emplace_back(relativePath, std::move(hash));
			}
		}
	}
	catch (const std::filesystem::filesystem_error& e) {
		LOG_ERROR("Filesystem error while scanning files: {}", e.what());
		m_callbacksHandler->onError();
	}

	LOG_DEBUG("Collected {} files for update check", result.size());
	return result;
}

}