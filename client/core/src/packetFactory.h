#pragma once 

#include <string>
#include <vector>
#include "utilities/crypto.h"

namespace core {

    class PacketFactory {
    public:
        PacketFactory() = default;

        // packets for server
        static std::pair<std::string, std::vector<unsigned char>> getAuthorizationPacket(const std::string& myNickname, const CryptoPP::RSA::PublicKey& myPublicKey);
        static std::pair<std::string, std::vector<unsigned char>> getLogoutPacket(const std::string& myNickname);
        static std::pair<std::string, std::vector<unsigned char>> getReconnectPacket(const std::string& myNickname, const std::string& myToken);
        static std::pair<std::string, std::vector<unsigned char>> getRequestUserInfoPacket(const std::string& myNickname, const std::string& userNickname);

        // packets for other user
        static std::vector<unsigned char> getConfirmationPacket(const std::string& myNicknameHash, const std::string& userNicknameHash, const std::string& uid);
        static std::pair<std::string, std::vector<unsigned char>> getStartOutgoingCallPacket(const std::string& myNickname, const std::string& userNickname, const CryptoPP::RSA::PublicKey& myPublicKey, const CryptoPP::RSA::PublicKey& userPublicKey, const CryptoPP::SecByteBlock& callKey);
        static std::pair<std::string, std::vector<unsigned char>> getStopOutgoingCallPacket(const std::string& myNickname, const std::string& userNickname);
        static std::pair<std::string, std::vector<unsigned char>> getAcceptCallPacket(const std::string& myNickname, const std::string& userNickname, const CryptoPP::RSA::PublicKey& myPublicKey, const CryptoPP::RSA::PublicKey& userPublicKey, const CryptoPP::SecByteBlock& callKey);
        static std::pair<std::string, std::vector<unsigned char>> getDeclineCallPacket(const std::string& myNickname, const std::string& userNickname);
        static std::pair<std::string, std::vector<unsigned char>> getEndCallPacket(const std::string& myNickname, const std::string& userNickname);
        static std::pair<std::string, std::vector<unsigned char>> getStartScreenSharingPacket(const std::string& myNickname, const std::string& userNickname);
        static std::pair<std::string, std::vector<unsigned char>> getStopScreenSharingPacket(const std::string& myNickname, const std::string& userNickname);
        static std::pair<std::string, std::vector<unsigned char>> getStartCameraSharingPacket(const std::string& myNickname, const std::string& userNickname);
        static std::pair<std::string, std::vector<unsigned char>> getStopCameraSharingPacket(const std::string& myNickname, const std::string& userNickname);
    };
}
