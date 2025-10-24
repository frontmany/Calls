#pragma once

#include <fstream>
#include <filesystem>
#include <string>

namespace updater {

std::string calculateFileHash(std::filesystem::path filepath);

}