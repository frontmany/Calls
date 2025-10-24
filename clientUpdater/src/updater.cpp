#include "updater.h"
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
		std::move([this]() {m_onError(); })
	)
{
}

void ClientUpdater::connect(const std::string& host, const std::string& port) {
	m_networkController.connect(host, port);
}

void ClientUpdater::disconnect() {
	m_networkController.disconnect();
}

void ClientUpdater::checkUpdates(const std::string& currentVersionNumber, OperationSystemType type) {
	auto pathsWithHashes = getFilePathsWithHashes();

	nlohmann::json jsonObject;

	jsonObject[VERSION] = currentVersionNumber;
	jsonObject[OPERATION_SYSTEM] = static_cast<int>(type);

	nlohmann::json filesArray = nlohmann::json::array();
	for (const auto& [path, hash] : pathsWithHashes) {
		nlohmann::json fileInfo;
		fileInfo[RELATIVE_FILE_PATH] = path.string();
		fileInfo[FILE_HASH] = hash;

		filesArray.push_back(fileInfo);
	}

	jsonObject[FILES] = filesArray;

	std::string jsonString = jsonObject.dump();

	Packet packet(static_cast<int>(PacketType::REQUEST));
	packet.setData(jsonString);

	m_networkController.sendPacket(packet);
}

void ClientUpdater::startUpdate() {
	nlohmann::json jsonObject;
	jsonObject[IS_ACCEPTED] = true;
	std::string jsonString = jsonObject.dump();

	Packet packet(static_cast<int>(PacketType::RESPOND));
	packet.setData(jsonString);

	m_networkController.sendPacket(packet);
}

void ClientUpdater::declineUpdate() {
	nlohmann::json jsonObject;
	jsonObject[IS_ACCEPTED] = false;
	std::string jsonString = jsonObject.dump();

	Packet packet(static_cast<int>(PacketType::RESPOND));
	packet.setData(jsonString);

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

bool ClientUpdater::checkRemoveJsonExists() {
	const std::filesystem::path tempDirectory = "update_temp";
	if (std::filesystem::exists(tempDirectory) && std::filesystem::is_directory(tempDirectory)) {
		const std::filesystem::path removeJsonPath = tempDirectory / "remove.json";

		if (std::filesystem::exists(removeJsonPath)) return true;
	}

	return false;
}

void ClientUpdater::processRemoveJson() {
	const std::filesystem::path tempDirectory = "update_temp";
	const std::filesystem::path removeJsonPath = tempDirectory / "remove.json";

	std::ifstream file(removeJsonPath);
	if (!file) {
		m_onError();
	}

	nlohmann::json jsonObject = nlohmann::json::parse(file);
	std::vector<std::string> filesToRemove = jsonObject["files"].get<std::vector<std::string>>();

	for (const auto& filename : filesToRemove) {
		if (std::filesystem::exists(filename)) {
			if (!std::filesystem::remove(filename))
				m_onError();
		}
	}

	std::filesystem::remove(removeJsonPath);
}

}