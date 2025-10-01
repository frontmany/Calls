#pragma once 

#include <string>

#include "rsa.h"

#include "crypto.h"

class Call;

class PacketsFactory {
public:
    PacketsFactory() = default;
    static std::string getFriendInfoSuccessPacket(const CryptoPP::RSA::PublicKey& publicKey, const std::string& nicknameHash);
    static std::string getParticipantLeavePacket(const std::string& participantNicknameHash, const std::string& groupCallNameHash);

private:
    static constexpr const char* PUBLIC_KEY = "publicKey";
    static constexpr const char* NICKNAME_HASH = "nicknameHash";
    static constexpr const char* GROUP_CALL_NAME_HASH = "groupCallNameHash";
};
