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
		std::move([this](CheckResult result) {m_onCheckResult(result); }),
		std::move([this]() {m_onUpdateLoaded(); }),
		std::move([this]() {m_onError(); }))
{
}

void ClientUpdater::connect(const std::string& host, const std::string& port) {
	m_networkController.connect(host, port);
}

void ClientUpdater::disconnect() {
	m_networkController.disconnect();
}

void ClientUpdater::checkUpdates(const std::string& currentVersionNumber) {
	nlohmann::json jsonObject;
	jsonObject[VERSION] = currentVersionNumber;
	Packet packet(static_cast<int>(PacketType::CHECK_UPDATES));
	packet.setData(jsonObject.dump());

	m_networkController.sendPacket(packet);
}

void ClientUpdater::startUpdate(OperationSystemType type) {
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

	Packet packet(static_cast<int>(PacketType::UPDATE_ACCEPT));
	packet.setData(jsonString);

	m_networkController.sendPacket(packet);
}

void ClientUpdater::declineUpdate() {
	Packet packet(static_cast<int>(PacketType::UPDATE_DECLINE));
	m_networkController.sendPacket(packet);
}

std::vector<std::pair<std::filesystem::path, std::string>> ClientUpdater::getFilePathsWithHashes() {
	std::vector<std::pair<std::filesystem::path, std::string>> result;

	try {
		std::filesystem::path currentPath = ".";
		std::filesystem::path basePath = std::filesystem::current_path();

		for (const auto& entry : std::filesystem::recursive_directory_iterator(currentPath)) {
			if (entry.is_regular_file()) {
				std::filesystem::path relativePath = std::filesystem::relative(entry.path(), basePath);
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