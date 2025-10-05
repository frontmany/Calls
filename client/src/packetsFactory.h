#pragma once 

#include <string>

#include "rsa.h"
#include "crypto.h"

namespace calls {

    class PacketsFactory {
    public:
        PacketsFactory() = default;
        static std::string getAuthorizationPacket(const std::string& myNickname, const CryptoPP::RSA::PublicKey& myPublicKey);
        static std::string getCreateCallPacket(const std::string& myNickname, const CryptoPP::RSA::PublicKey& myPublicKey, const std::string& friendNickname, const CryptoPP::RSA::PublicKey& friendPublicKey, const CryptoPP::SecByteBlock& callKey);
        static std::string getCallingEndPacket(const std::string& myNickname, const std::string& friendNickname);
        static std::string getRequestFriendInfoPacket(const std::string& myNickname, const std::string& estimatedFriendNickname);
        static std::string getDeclineCallPacket(const std::string& myNickname, const std::string& friendNickname);
        static std::string getDeclineAllCallsPacket(const std::string& myNickname, const std::vector<std::string>& nicknames);
        static std::string getAcceptCallPacket(const std::string& myNickname, const std::string& friendNickname);
        static std::string getEndCallPacket(const std::string& myNickname);
        static std::string getLogoutPacket(const std::string& myNickname);
    };
}
