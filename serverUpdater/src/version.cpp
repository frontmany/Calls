#include "version.h"
#include <sstream>
#include <vector>
#include <algorithm>
#include <cctype>

Version::Version(const std::string& versionAsString)
    : m_version(versionAsString)
{
}

const std::string& Version::getAsString() const
{
    return m_version;
}

bool Version::isValidVersion() const
{
    if (m_version.empty() || isVersionLost()) {
        return false;
    }

    for (char c : m_version) {
        if (!std::isdigit(c) && c != '.') {
            return false;
        }
    }

    bool hasDigit = false;
    for (char c : m_version) {
        if (std::isdigit(c)) {
            hasDigit = true;
            break;
        }
    }

    return hasDigit;
}

bool Version::isVersionLost() const
{
    return m_version == VERSION_LOST;
}

std::vector<int> Version::parseVersionParts() const
{
    std::vector<int> parts;

    if (!isValidVersion() || isVersionLost()) {
        parts.push_back(-1);
        return parts;
    }

    std::stringstream ss(m_version);
    std::string token;

    while (std::getline(ss, token, '.')) {
        try {
            parts.push_back(std::stoi(token));
        }
        catch (const std::exception&) {
            parts.clear();
            parts.push_back(-1);
            break;
        }
    }

    return parts;
}

bool Version::operator<(const Version& other) const
{
    if (isVersionLost() && other.isVersionLost()) {
        return false;
    }
    if (isVersionLost()) {
        return true;
    }
    if (other.isVersionLost()) {
        return false;
    }

    std::vector<int> parts1 = parseVersionParts();
    std::vector<int> parts2 = other.parseVersionParts();

    if (parts1[0] == -1 && parts2[0] == -1) {
        return false;
    }
    if (parts1[0] == -1) {
        return true;
    }
    if (parts2[0] == -1) {
        return false;
    }

    size_t maxSize = std::max(parts1.size(), parts2.size());
    for (size_t i = 0; i < maxSize; ++i) {
        int num1 = (i < parts1.size()) ? parts1[i] : 0;
        int num2 = (i < parts2.size()) ? parts2[i] : 0;

        if (num1 < num2) {
            return true;
        }
        else if (num1 > num2) {
            return false;
        }
    }

    return false;
}

bool Version::operator>(const Version& other) const {
    return other < *this;
}

bool Version::operator==(const Version& other) const {
    return !(*this < other) && !(other < *this);
}

bool Version::operator!=(const Version& other) const {
    return !(*this == other);
}

bool Version::operator<=(const Version& other) const {
    return !(other < *this);
}

bool Version::operator>=(const Version& other) const {
    return !(*this < other);
}

bool Version::operator<(const std::string& other) const
{
    return *this < Version(other);
}

bool Version::operator>(const std::string& other) const
{
    return *this > Version(other);
}

bool Version::operator==(const std::string& other) const
{
    return *this == Version(other);
}

bool Version::operator!=(const std::string& other) const
{
    return *this != Version(other);
}

bool Version::operator<=(const std::string& other) const
{
    return *this <= Version(other);
}

bool Version::operator>=(const std::string& other) const
{
    return *this >= Version(other);
}