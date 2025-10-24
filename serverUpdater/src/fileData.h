#pragma once

#include <filesystem>
#include <string>

struct FileData {
	std::filesystem::path filePath;
	std::string fileHash;
};