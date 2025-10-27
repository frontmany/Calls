#pragma once 

#include <filesystem>

#include "connection.h"
#include "packet.h"

constexpr const char* MAJOR_UPDATE = "major";
constexpr const char* MINOR_UPDATE = "minor";
const std::filesystem::path versionsDirectory = "versions";

const std::string& compareVersions(const std::string& v1, const std::string& v2);
std::filesystem::path findLatestVersionPath();

static void onUpdatesCheck(ConnectionPtr connection, Packet&& packet);
static void onUpdateAccepted(ConnectionPtr connection, Packet&& packet);
void runServerUpdater();
int main();