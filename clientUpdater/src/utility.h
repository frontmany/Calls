#pragma once

#include <cstdint>
#include <filesystem>
#include <string>

namespace callifornia {
	namespace updater {

std::string calculateFileHash(std::filesystem::path filepath);
uint64_t scramble(uint64_t inputNumber);
	}
}