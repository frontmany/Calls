#pragma once 

#include <string>
#include <unordered_map>

#include "rsa.h"

#include "crypto.h"
#include "call.h"

namespace calls {

class PacketsFactory {
public:
    PacketsFactory() = default;
    static std::string getAuthorizationPacket(const CryptoPP::RSA::PublicKey& myPublicKey, const std::string& myNickname);
    static std::string getCreateCallPacket(const CryptoPP::RSA::PublicKey& myPublicKey, const std::string& myNickname, const Call& call);
    static std::string getCallingEndPacket(const std::string& myNickname, const std::string& friendNicknameHash);
    static std::string getRequestFriendInfoPacket(const std::string& friendNickname);
    static std::string getDeclineCallPacket(const std::string& friendNickname);
    static std::string getAcceptCallPacket(const std::string& friendNickname);

    static std::string getCreateGroupCallPacket(const std::string& myNickname, const std::string& groupCallName);
    static std::string getJoinAllowedPacket(const CryptoPP::SecByteBlock& groupCallKey, const std::unordered_map<std::string, CryptoPP::RSA::PublicKey>& participants, const std::string& myNickname, const CryptoPP::RSA::PublicKey& myPublicKey, const std::string& groupCallName, const std::string& nicnameTo, const CryptoPP::RSA::PublicKey& publicKeyTo);
    static std::string getGroupCallNewParticipantPacket(const std::string& nicknameTo, const CryptoPP::RSA::PublicKey& publicKeyTo, const std::string newParticipantNickname);
    static std::string getJoinDeclinedPacket(const std::string& nicknameTo, const CryptoPP::RSA::PublicKey& publicKeyTo, const std::string groupCallName);
    static std::string getEndGroupCallPacket(const std::string& myNickname, const std::string groupCallName);
    static std::string getLeaveGroupCallPacket(const std::string& myNickname, const std::string groupCallName);
    static std::string getJoinGroupCallPacket(const std::string& myNickname, const CryptoPP::RSA::PublicKey& myPublicKey, const std::string& nicknameHashTo, const CryptoPP::RSA::PublicKey& publicKeyTo);
    static std::string getCheckGroupCallExistencePacket(const std::string& groupCallName);

private:
    static constexpr const char* CALL_KEY = "callKey";
    static constexpr const char* PACKET_KEY = "packetKey";
    static constexpr const char* PUBLIC_KEY = "publicKey";
    static constexpr const char* NICKNAME = "nickname";
    static constexpr const char* INITIATOR_NICKNAME = "initiatorNickname";
    static constexpr const char* NICKNAME_HASH = "nicknameHash";
    static constexpr const char* NICKNAME_HASH_TO = "nicknameHashTo";
    static constexpr const char* GROUP_CALL_NAME_HASH = "groupCallNameHash";
    static constexpr const char* GROUP_CALL_NAME = "groupCallName";
    static constexpr const char* GROUP_CALL_KEY = "groupCallKey";
    static constexpr const char* PARTICIPANTS_ARRAY = "participantsArray";
};

}
