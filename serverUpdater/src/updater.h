#pragma once 

#include <filesystem>

#include "connection.h"
#include "packet.h"

constexpr const char* MAJOR_UPDATE = "major";
constexpr const char* MINOR_UPDATE = "minor";
const std::filesystem::path versionsDirectory = "versions";

std::string findLatestVersion(const std::filesystem::path& versionsDirectory);
bool isValidVersion(const std::string& version);
bool isVersionNewer(const std::string& version1, const std::string& version2);
std::vector<int> parseVersion(const std::string& version);
const std::string& compareVersions(const std::string& v1, const std::string& v2);
std::filesystem::path findLatestVersionPath();

static void onUpdatesCheck(ConnectionPtr connection, Packet&& packet);
static void onUpdateAccepted(ConnectionPtr connection, Packet&& packet);
int main();