#include "updater.h"

namespace callifornia {
	namespace updater {

		Updater::Updater()
	: m_networkController(
		[this](UpdateCheckResult result) {m_queue.push_back([this, result]() {m_state = State::AWAITING_START_UPDATE; m_eventListener->onUpdateCheckResult(result); }); },
		[this](double progress) {m_queue.push_back([this, progress]() {m_eventListener->onLoadingProgress(progress); m_processingProgress = true; }); },
		[this](bool emptyUpdate) {m_queue.push_back([this, emptyUpdate]() {m_state = State::AWAITING_UPDATES_CHECK; m_eventListener->onUpdateLoaded(emptyUpdate); m_processingProgress = false; }); },
		[this]() {m_queue.push_back([this]() {m_state = State::AWAITING_UPDATES_CHECK;});  },
		[this]() {m_queue.push_back([this]() {m_eventListener->onNetworkError(); }); })
{
}

Updater::~Updater() {
	m_running = false;
	if (m_queueProcessingThread.joinable()) {
		m_queueProcessingThread.join();
	}
}

void Updater::init(std::shared_ptr<EventListener> eventListener) {
	m_eventListener = eventListener;
}

bool Updater::connect(const std::string& host, const std::string& port) {
	m_serverHost = host;
	m_serverPort = port;

	if (m_state != State::DISCONNECTED || m_running) return false;

	m_state = State::AWAITING_SERVER_RESPONSE;
	m_networkController.connect(host, port);

	m_running = true;
	m_queueProcessingThread = std::thread([this]() {processQueue(); });

	return true;
}

void Updater::disconnect() {
	m_networkController.requestShutdown();
	m_networkController.disconnect();
	m_state = State::DISCONNECTED;

	m_running = false;
	if (m_queueProcessingThread.joinable()) {
		m_queueProcessingThread.join();
	}

	m_queue.clear();
}

		void Updater::processQueue() {
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

void Updater::checkUpdates(const std::string& currentVersionNumber) {
	m_checkUpdatesFuture = std::async(std::launch::async, [this, currentVersionNumber]() {
		auto startTime = std::chrono::steady_clock::now();
		auto timeout = std::chrono::seconds(5);

		while (std::chrono::steady_clock::now() - startTime < timeout) {
			if (m_state == State::AWAITING_UPDATES_CHECK) {
				break;
			}
		}

		if (m_state != State::AWAITING_UPDATES_CHECK) return;

		nlohmann::json jsonObject;
		jsonObject["version"] = currentVersionNumber;
		Packet packet(static_cast<int>(PacketType::CHECK_UPDATES), jsonObject.dump());

		m_networkController.sendPacket(packet);

		m_state = State::AWAITING_SERVER_RESPONSE;
	});
}

bool Updater::startUpdate(OperationSystemType type) {
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

bool Updater::isConnected() {
	return m_state != State::DISCONNECTED;
}

bool Updater::isAwaitingServerResponse() {
	return m_state == State::AWAITING_SERVER_RESPONSE;
}

bool Updater::isAwaitingCheckUpdatesFunctionCall() {
	return m_state == State::AWAITING_UPDATES_CHECK;
}

bool Updater::isAwaitingStartUpdateFunctionCall() {
	return m_state == State::AWAITING_START_UPDATE;
}

const std::string& Updater::getServerHost() {
	return m_serverHost;
}

const std::string& Updater::getServerPort() {
	return m_serverPort;
}

std::string Updater::normalizePath(const std::filesystem::path& path) {
	std::string normalized = path.generic_string();

	if (normalized.find("./") == 0) {
		normalized = normalized.substr(2);
	}

	return normalized;
}

std::vector<std::pair<std::filesystem::path, std::string>> Updater::getFilePathsWithHashes() {
	std::vector<std::pair<std::filesystem::path, std::string>> result;

	try {
		std::filesystem::path currentPath = std::filesystem::current_path();

		for (const auto& entry : std::filesystem::recursive_directory_iterator(currentPath)) {
			if (entry.is_regular_file()) {
				std::filesystem::path relativePath = std::filesystem::relative(entry.path(), currentPath);

				if (entry.path().filename() == "config.json" ||
					entry.path().filename() == "update_applier.exe" ||
					entry.path().filename() == "update_applier" ||
					entry.path().filename() == "callifornia.exe.manifest" ||
					entry.path().filename() == "unins000.dat" ||
					entry.path().filename() == "unins000.exe") {
					continue;
				}

				if (relativePath.string().find("update_temp") == 0 || relativePath.string().find("logs") == 0) {
					continue;
				}

				std::string hash = calculateFileHash(entry.path());
				std::string normalizedPathStr = normalizePath(relativePath);
				std::filesystem::path normalizedPath(normalizedPathStr);

				result.emplace_back(normalizedPath, std::move(hash));
			}
		}
	}
	catch (const std::filesystem::filesystem_error& e) {
		m_eventListener->onNetworkError();
	}

	return result;
}

	}
}