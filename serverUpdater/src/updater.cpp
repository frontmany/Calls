#include "updater.h"

#include <regex>

#include "networkController.h"
#include "operationSystemType.h"
#include "jsonTypes.h"
#include "checkResult.h"
#include "version.h"
#include "logger.h"
#include "utility.h"

#include "json.hpp"

std::pair<std::filesystem::path, std::string> findLatestVersion() {
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

                if (currentVersion > latestVersion) {
                    latestVersion = currentVersion;
                    latestVersionPath = entry.path();
                }
            }
        }
    }

    return std::make_pair(latestVersionPath, latestVersion);
}

void onUpdatesCheck(ConnectionPtr connection, Packet&& packet) {
    nlohmann::json jsonObject = nlohmann::json::parse(packet.data());

    Version version(jsonObject[VERSION].get<std::string>());

    Packet packetResponse;

    if (version != VERSION_LOST) {
        bool hasMajorUpdate = false;
        Version latestVersion = version;

        for (const auto& entry : std::filesystem::directory_iterator(versionsDirectory)) {
            if (entry.is_directory()) {
                std::filesystem::path versionJsonPath = entry.path() / "version.json";

                if (std::filesystem::exists(versionJsonPath) &&
                    std::filesystem::is_regular_file(versionJsonPath)) {
                    std::ifstream file(versionJsonPath);
                    nlohmann::json versionJson = nlohmann::json::parse(file);

                    Version currentVersion(versionJson[VERSION].get<std::string>());
                    std::string updateType = versionJson[UPDATE_TYPE].get<std::string>();


                    if (currentVersion > version) {

                        if (updateType == MAJOR_UPDATE) {
                            hasMajorUpdate = true;
                        }

                        if (currentVersion > latestVersion) {
                            latestVersion = currentVersion;
                        }
                    }
                }
            }
        }

        if (latestVersion == version) {
            packetResponse.setType(static_cast<int>(CheckResult::UPDATE_NOT_NEEDED));
        }
        else if (hasMajorUpdate) {
            packetResponse.setType(static_cast<int>(CheckResult::REQUIRED_UPDATE));
        }
        else {
            packetResponse.setType(static_cast<int>(CheckResult::POSSIBLE_UPDATE));
        }
    }
    else {
        packetResponse.setType(static_cast<int>(CheckResult::REQUIRED_UPDATE));
    }

    connection->sendPacket(packetResponse);
}

void onUpdateAccepted(ConnectionPtr connection, Packet&& packet) {
    nlohmann::json jsonObject = nlohmann::json::parse(packet.data());

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

        auto [latestVersionPath, latestVersion] = findLatestVersion();

        std::vector<std::filesystem::path> filesToDelete;
        std::vector<std::filesystem::path> filesToSend;

        std::unordered_map<std::filesystem::path, std::string> newVersionFiles;
        for (const auto& entry : std::filesystem::recursive_directory_iterator(latestVersionPath)) {
            if (entry.is_regular_file() && entry.path().filename() != "version.json") {
                std::filesystem::path relativePath = std::filesystem::relative(entry.path(), latestVersionPath);
                std::string fileHash = calculateFileHash(entry.path());
                newVersionFiles[relativePath] = fileHash;
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
            auto fullPath = latestVersionPath / filePath;
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
        responseJson[VERSION] = latestVersion;

        nlohmann::json filesToDeleteArray = nlohmann::json::array();
        for (const auto& filePath : filesToDelete) {
            filesToDeleteArray.push_back(filePath.string());
        }
        responseJson[FILES_TO_DELETE] = filesToDeleteArray;

        Packet packet;
        packet.setData(responseJson.dump());

        connection->sendPacket(packet);

        for (const auto& path : serverPathsToSend) 
            connection->sendFile(path);
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