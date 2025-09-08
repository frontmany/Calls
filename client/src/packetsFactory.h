#pragma once 

#include <string>

#include "rsa.h"

#include "crypto.h"

class Call;

class PacketsFactory {
public:
    PacketsFactory() = default;
    static std::string getAuthorizationPacket(const CryptoPP::RSA::PublicKey& myPublicKey, const std::string& myNickname);
    static std::string getCreateCallPacket(const CryptoPP::RSA::PublicKey& myPublicKey, const std::string& myNickname, const Call& call);
    static std::string getRequestFriendInfoPacket(const std::string& friendNickname);
    static std::string getDeclineCallPacket(const std::string& friendNickname);
    static std::string getAcceptCallPacket(const std::string& friendNickname);

private:
    static constexpr const char* CALL_KEY = "callKey";
    static constexpr const char* PACKET_KEY = "packetKey";
    static constexpr const char* PUBLIC_KEY = "publicKey";
    static constexpr const char* NICKNAME = "nickname";
    static constexpr const char* NICKNAME_HASH = "nicknameHash";
};
