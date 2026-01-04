#pragma once

#include <filesystem>
#include <string>

namespace serverUpdater
{
namespace utilities
{
std::string calculateFileHash(std::filesystem::path filepath);
uint64_t scramble(uint64_t inputNumber);
}
}