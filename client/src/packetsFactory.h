#pragma once 

#include <string>

#include "rsa.h"
#include "crypto.h"

namespace calls {

    class PacketsFactory {
    public:
        PacketsFactory() = default;
        static std::pair<std::string, std::string> getAuthorizationPacket(const std::string& myNickname, const CryptoPP::RSA::PublicKey& myPublicKey);
        static std::pair<std::string, std::string> getCreateCallPacket(const std::string& myNickname, const CryptoPP::RSA::PublicKey& myPublicKey, const std::string& friendNickname, const CryptoPP::RSA::PublicKey& friendPublicKey, const CryptoPP::SecByteBlock& callKey);
        static std::pair<std::string, std::string> getCallingEndPacket(const std::string& myNickname, const std::string& friendNickname);
        static std::pair<std::string, std::string> getRequestFriendInfoPacket(const std::string& myNickname, const std::string& estimatedFriendNickname);
        static std::pair<std::string, std::string> getDeclineCallPacket(const std::string& myNickname, const std::string& friendNickname);
        static std::pair<std::string, std::string> getDeclineAllCallsPacket(const std::string& myNickname, const std::vector<std::string>& nicknames);
        static std::pair<std::string, std::string> getAcceptCallPacket(const std::string& myNickname, const std::string& friendNicknameToAccept, const std::vector<std::string>& nicknamesToDecline);
        static std::pair<std::string, std::string> getAcceptCallPacketAndEndCalling(const std::string& myNickname, const std::string& friendNicknameToAccept, const std::vector<std::string>& nicknamesToDecline, const std::string& friendNicknameToEndCalling);
        static std::pair<std::string, std::string> getEndCallPacket(const std::string& myNickname);
        static std::pair<std::string, std::string> getLogoutPacket(const std::string& myNickname, const std::vector<std::string>& nicknamesToDecline = {}, const std::string& friendNicknameToEndCalling = "");
    };
}
