#include "updater.h"

#include <regex>

#include "networkController.h"
#include "operationSystemType.h"
#include "jsonTypes.h"
#include "checkResult.h"
#include "logger.h"
#include "utility.h"

#include "json.hpp"

const std::string& compareVersions(const std::string& v1, const std::string& v2) {
    std::vector<int> parts1, parts2;
    std::stringstream ss1(v1), ss2(v2);
    std::string part;

    while (std::getline(ss1, part, '.')) {
        parts1.push_back(std::stoi(part));
    }

    while (std::getline(ss2, part, '.')) {
        parts2.push_back(std::stoi(part));
    }

    for (size_t i = 0; i < std::min(parts1.size(), parts2.size()); ++i) {
        if (parts1[i] > parts2[i]) return v1;
        if (parts1[i] < parts2[i]) return v2;
    }

    return parts1.size() > parts2.size() ? v1 : v2;
}

std::filesystem::path findLatestVersionPath() {
    std::string latestVersion;
    std::filesystem::path latestVersionPath;

    for (const auto& entry : std::filesystem::directory_iterator(versionsDirectory)) {
        if (entry.is_directory()) {
            std::filesystem::path versionJsonPath = entry.path() / "version.json";

            if (std::filesystem::exists(versionJsonPath) &&
                std::filesystem::is_regular_file(versionJsonPath)) {
                std::ifstream file(versionJsonPath);
                nlohmann::json versionJson = nlohmann::json::parse(file);

                std::string currentVersion = versionJson[VERSION].get<std::string>();
                std::string updateType = versionJson[UPDATE_TYPE].get<std::string>();

                if (latestVersion.empty())
                    latestVersion = currentVersion;

                if (const std::string& latest = compareVersions(currentVersion, latestVersion); currentVersion == latest) {
                    latestVersion = currentVersion;
                    latestVersionPath = entry.path();
                }
            }
        }
    }

    return latestVersionPath;
}

void onUpdatesCheck(ConnectionPtr connection, Packet&& packet) {
    nlohmann::json jsonObject;
    std::string version = jsonObject[VERSION].get<std::string>();

    if (version != VERSION_LOST) {
        bool hasMajorUpdate = false;
        std::string latestVersion = version;

        for (const auto& entry : std::filesystem::directory_iterator(versionsDirectory)) {
            if (entry.is_directory()) {
                std::filesystem::path versionJsonPath = entry.path() / "version.json";

                if (std::filesystem::exists(versionJsonPath) &&
                    std::filesystem::is_regular_file(versionJsonPath)) {
                    std::ifstream file(versionJsonPath);
                    nlohmann::json versionJson = nlohmann::json::parse(file);

                    std::string currentVersion = versionJson[VERSION].get<std::string>();
                    std::string updateType = versionJson[UPDATE_TYPE].get<std::string>();


                    if (const std::string& latest = compareVersions(currentVersion, version); latest == version) {

                        if (updateType == MAJOR_UPDATE) {
                            hasMajorUpdate = true;
                        }

                        if (const std::string& latest = compareVersions(currentVersion, latestVersion); currentVersion == latest) {
                            latestVersion = currentVersion;
                        }
                    }
                }
            }
        }

        if (latestVersion == version) {
            Packet packet(static_cast<int>(CheckResult::UPDATE_NOT_NEEDED));
            connection->sendUpdateNotNeededPacket(packet);
        }
        else if (hasMajorUpdate) {
            Packet packet(static_cast<int>(CheckResult::REQUIRED_UPDATE));
            connection->sendUpdateRequiredPacket(packet);
        }
        else {
            Packet packet(static_cast<int>(CheckResult::POSSIBLE_UPDATE));
            connection->sendUpdatePossiblePacket(packet);
        }
    }
    else {
        Packet packet(static_cast<int>(CheckResult::REQUIRED_UPDATE));
        connection->sendUpdateRequiredPacket(packet);
    }
}

void onUpdateAccepted(ConnectionPtr connection, Packet&& packet) {
    std::string jsonString = packet.data();
    nlohmann::json jsonObject = nlohmann::json::parse(jsonString);

    OperationSystemType osType;
    if (jsonObject.contains(OPERATION_SYSTEM)) {
        osType = static_cast<OperationSystemType>(
            jsonObject[OPERATION_SYSTEM].get<int>()
            );
    }
    else {
        DEBUG_LOG("Missing OPERATION_SYSTEM field in packet");
        return;
    }

    if (jsonObject.contains(FILES) && jsonObject[FILES].is_array()) {
        std::vector<std::pair<std::filesystem::path, std::string>> filePathsWithHashes;

        for (const auto& fileInfo : jsonObject[FILES]) {
            if (fileInfo.contains(RELATIVE_FILE_PATH) && fileInfo.contains(FILE_HASH)) {
                std::string relativePath = fileInfo[RELATIVE_FILE_PATH].get<std::string>();
                std::string fileHash = fileInfo[FILE_HASH].get<std::string>();

                filePathsWithHashes.emplace_back(relativePath, fileHash);
            }
            else {
                DEBUG_LOG("Incomplete file information in packet");
                return;
            }
        }

        std::filesystem::path latestVersionPath = findLatestVersionPath();

        std::vector<std::filesystem::path> filesToDelete;
        std::vector<std::filesystem::path> filesToSend;
        std::string version; 

        std::unordered_map<std::filesystem::path, std::string> newVersionFiles;
        for (const auto& entry : std::filesystem::recursive_directory_iterator(latestVersionPath)) {
            if (entry.is_regular_file() && entry.path().filename() != "version.json") {
                std::filesystem::path relativePath = std::filesystem::relative(entry.path(), latestVersionPath);
                std::string fileHash = calculateFileHash(entry.path());
                newVersionFiles[relativePath] = fileHash;
            }
            else {
                std::ifstream file(entry.path());
                if (file.is_open()) {
                    try {
                        nlohmann::json jsonObject;
                        file >> jsonObject;
                        if (jsonObject.contains(VERSION)) {
                            version = jsonObject[VERSION].get<std::string>();
                        }
                    }
                    catch (const nlohmann::json::exception& e) {
                        DEBUG_LOG("JSON parsing error onUpdateAccepted");
                    }
                    file.close();
                }
            }
        }

        std::unordered_set<std::filesystem::path> clientFiles;
        for (const auto& [clientRelativePath, clientHash] : filePathsWithHashes) {
            clientFiles.insert(clientRelativePath);

            auto it = newVersionFiles.find(clientRelativePath);
            if (it == newVersionFiles.end()) {
                filesToDelete.push_back(clientRelativePath);
            }
            else if (it->second != clientHash) {
                filesToSend.push_back(clientRelativePath);
            }
        }

        for (const auto& [serverRelativePath, serverHash] : newVersionFiles) {
            if (clientFiles.find(serverRelativePath) == clientFiles.end()) {
                filesToSend.push_back(serverRelativePath);
            }
        }

        nlohmann::json responseJson;

        std::vector<std::filesystem::path> serverPathsToSend;

        nlohmann::json filesToDownloadArray = nlohmann::json::array();
        for (const auto& filePath : filesToSend) {
            auto fullPath = versionsDirectory / latestVersionPath / filePath;
            if (std::filesystem::exists(fullPath)) {
                serverPathsToSend.push_back(fullPath);

                uint64_t fileSize = std::filesystem::file_size(fullPath);
                std::string fileHash = newVersionFiles[filePath];

                nlohmann::json fileEntry;
                fileEntry[RELATIVE_FILE_PATH] = filePath.string();
                fileEntry[FILE_SIZE] = fileSize;
                fileEntry[FILE_HASH] = fileHash;

                filesToDownloadArray.push_back(fileEntry);
            }
        }
        responseJson[FILES_TO_DOWNLOAD] = filesToDownloadArray;
        responseJson[VERSION] = version;

        nlohmann::json filesToDeleteArray = nlohmann::json::array();
        for (const auto& filePath : filesToDelete) {
            filesToDeleteArray.push_back(filePath.string());
        }
        responseJson[FILES_TO_DELETE] = filesToDeleteArray;

        Packet packet;
        packet.setData(responseJson.dump());

        connection->sendUpdate(packet, serverPathsToSend);
    }
    else {
        DEBUG_LOG("Missing FILES field in packet");
        return;
    }
}

void runServerUpdater() {
    NetworkController networkController(8081,
        [](ConnectionPtr connection, Packet&& packet) {onUpdatesCheck(connection, std::move(packet)); },
        [](ConnectionPtr connection, Packet&& packet) {onUpdateAccepted(connection, std::move(packet)); }
    );

    DEBUG_LOG("[SERVER] server started!");

    networkController.start();
}


int main() {
    runServerUpdater();
}