#pragma once 

#include <string>
#include <vector>

#include "jsonType.h"

namespace serverUpdater
{
class Version {
public:
    explicit Version(const std::string& versionAsString = VERSION_LOST);
    const std::string& getAsString() const;

    bool operator<(const Version& other) const;
    bool operator>(const Version& other) const;
    bool operator==(const Version& other) const;
    bool operator!=(const Version& other) const;
    bool operator<=(const Version& other) const;
    bool operator>=(const Version& other) const;

    bool operator<(const std::string& other) const;
    bool operator>(const std::string& other) const;
    bool operator==(const std::string& other) const;
    bool operator!=(const std::string& other) const;
    bool operator<=(const std::string& other) const;
    bool operator>=(const std::string& other) const;

private:
    bool isValidVersion() const;
    bool isVersionLost() const;
    std::vector<int> parseVersionParts() const;

private:
    std::string m_version;
};
}