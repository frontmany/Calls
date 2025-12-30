#pragma once 

#include <string>
#include <vector>

#include "rsa.h"
#include "utilities/crypto.h"

namespace calls {

    class PacketFactory {
    public:
        PacketFactory() = default;
        static std::pair<std::string, std::vector<unsigned char>> getAuthorizationPacket(const std::string& myNickname, const CryptoPP::RSA::PublicKey& myPublicKey);
        static std::pair<std::string, std::vector<unsigned char>> getLogoutPacket(const std::string& myNickname);
        static std::pair<std::string, std::vector<unsigned char>> getRequestFriendInfoPacket(const std::string& myNickname, const std::string& estimatedFriendNickname);
        static std::pair<std::string, std::vector<unsigned char>> getReconnectPacket(const std::string& myToken);


        static std::pair<std::string, std::vector<unsigned char>> getStartCallingPacket(const std::string& myNickname, const CryptoPP::RSA::PublicKey& myPublicKey, const std::string& friendNickname, const CryptoPP::RSA::PublicKey& friendPublicKey, const CryptoPP::SecByteBlock& callKey);
        static std::pair<std::string, std::vector<unsigned char>> getStopCallingPacket(const std::string& myNickname, const std::string& friendNickname);
        static std::pair<std::string, std::vector<unsigned char>> getDeclineCallPacket(const std::string& myNickname, const std::string& friendNickname);
        static std::pair<std::string, std::vector<unsigned char>> getAcceptCallPacket(const std::string& myNickname, const std::string& friendNicknameToAccept);
        static std::pair<std::string, std::vector<unsigned char>> getEndCallPacket(const std::string& myNickname);
        static std::pair<std::string, std::vector<unsigned char>> getStartScreenSharingPacket(const std::string& myNickname, const std::string& friendNicknameHash);
        static std::pair<std::string, std::vector<unsigned char>> getStopScreenSharingPacket(const std::string& myNickname, const std::string& friendNicknameHash);
        static std::pair<std::string, std::vector<unsigned char>> getStartCameraSharingPacket(const std::string& myNickname, const std::string& friendNicknameHash);
        static std::pair<std::string, std::vector<unsigned char>> getStopCameraSharingPacket(const std::string& myNickname, const std::string& friendNicknameHash);


        static std::vector<unsigned char> getConfirmationPacket(const std::string& myNickname, const std::string& nicknameHashTo, const std::string& uuid);
    };
}
