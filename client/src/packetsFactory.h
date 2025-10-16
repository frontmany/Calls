#pragma once 

#include <string>

#include "rsa.h"
#include "crypto.h"

namespace calls {

    class PacketsFactory {
    public:
        PacketsFactory() = default;
        static std::pair<std::string, std::string> getAuthorizationPacket(const std::string& myNickname, const CryptoPP::RSA::PublicKey& myPublicKey);
        static std::pair<std::string, std::string> getLogoutPacket(const std::string& myNickname);
        static std::pair<std::string, std::string> getRequestFriendInfoPacket(const std::string& myNickname, const std::string& estimatedFriendNickname);


        static std::pair<std::string, std::string> getStartCallingPacket(const std::string& myNickname, const CryptoPP::RSA::PublicKey& myPublicKey, const std::string& friendNickname, const CryptoPP::RSA::PublicKey& friendPublicKey, const CryptoPP::SecByteBlock& callKey);
        static std::pair<std::string, std::string> getStopCallingPacket(const std::string& myNickname, const std::string& friendNickname, bool needConfirmation);
        static std::pair<std::string, std::string> getDeclineCallPacket(const std::string& myNickname, const std::string& friendNickname, bool needConfirmation);
        static std::pair<std::string, std::string> getAcceptCallPacket(const std::string& myNickname, const std::string& friendNicknameToAccept);
        static std::pair<std::string, std::string> getEndCallPacket(const std::string& myNickname, const std::string& friendNickname, bool needConfirmation);

        static std::string getConfirmationPacket(const std::string& myNickname, const std::string& nicknameHashTo, const std::string& uuid);
    };
}
