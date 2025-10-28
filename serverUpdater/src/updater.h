#pragma once 

#include <filesystem>

#include "connection.h"
#include "packet.h"

constexpr const char* MAJOR_UPDATE = "major";
constexpr const char* MINOR_UPDATE = "minor";
const std::filesystem::path versionsDirectory = "versions";

std::pair<std::filesystem::path, std::string> findLatestVersion();

static void onUpdatesCheck(ConnectionPtr connection, Packet&& packet);
static void onUpdateAccepted(ConnectionPtr connection, Packet&& packet);
void runServerUpdater();
int main();