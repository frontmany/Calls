#include "utility.h"

namespace updater {

std::string calculateFileHash(std::filesystem::path filepath) {
	std::ifstream file(filepath, std::ios::binary);
	if (!file.is_open()) {
		return "";
	}

	std::string content((std::istreambuf_iterator<char>(file)),
		std::istreambuf_iterator<char>());

	std::size_t hash_value = std::hash<std::string>{}(content);

	std::stringstream ss;
	ss << std::hex << std::setw(16) << std::setfill('0') << hash_value;
	return ss.str();
}

}