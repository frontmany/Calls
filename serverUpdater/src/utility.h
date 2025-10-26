#pragma once

#include <fstream>
#include <filesystem>
#include <string>

std::string calculateFileHash(std::filesystem::path filepath);
