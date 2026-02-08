#pragma once 

#include <string>
#include <vector>
#include "utilities/crypto.h"

namespace core::logic 
{
    class PacketFactory {
    public:
        PacketFactory() = default;

        static std::vector<unsigned char> getNicknamePacket(const std::string& myNickname);
        static std::vector<unsigned char> getTwoNicknamesPacket(const std::string& myNickname, const std::string& userNickname);

        static std::vector<unsigned char> getAuthorizationPacket(const std::string& myNickname, const CryptoPP::RSA::PublicKey& myPublicKey, uint16_t myUdpPort);
        static std::vector<unsigned char> getReconnectPacket(const std::string& myNickname, const std::string& myToken, uint16_t myUdpPort);
        static std::vector<unsigned char> getRequestUserInfoPacket(const std::string& myNickname, const CryptoPP::RSA::PublicKey& myPublicKey, const std::string& userNickname);
        
        static std::vector<unsigned char> getCallPacketWithKeys(
            const std::string& myNickname,
            const std::string& userNickname,
            const CryptoPP::RSA::PublicKey& myPublicKey,
            const CryptoPP::RSA::PublicKey& userPublicKey,
            const CryptoPP::SecByteBlock& callKey
        );
    };
}