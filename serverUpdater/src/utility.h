#pragma once

#include <filesystem>
#include <string>

std::string calculateFileHash(std::filesystem::path filepath);
uint64_t scramble(uint64_t inputNumber);