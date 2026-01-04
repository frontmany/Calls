#pragma once 

#include <string>
#include <optional>
#include "utilities/crypto.h"
#include "jsonType.h"
#include "json.hpp"

namespace server
{
    class PacketFactory {
public:
    PacketFactory() = default;
    
    static std::vector<unsigned char> getConfirmationPacket(const std::string& uid, const std::string& receiverNicknameHash);
    static std::vector<unsigned char> getAuthorizationResultPacket(bool authorized, const std::string& uid, const std::string& receiverNicknameHash, std::optional<std::string> receiverToken = std::nullopt);
    static std::vector<unsigned char> getReconnectionResultPacket(bool reconnectedSuccessfully, const std::string& uid, const std::string& receiverNicknameHash, const std::string& receiverToken, std::optional<bool> activeCall = std::nullopt);
    static std::vector<unsigned char> getUserInfoResultPacket(bool userInfoFound, const std::string& uid, const std::string& userNicknameHash, std::optional<CryptoPP::RSA::PublicKey> userPublicKey = std::nullopt);
    static std::pair<std::string, std::vector<unsigned char>> getConnectionDownWithUserPacket(const std::string& userNicknameHash);
    static std::pair<std::string, std::vector<unsigned char>> getConnectionRestoredWithUserPacket(const std::string& userNicknameHash);
    static std::pair<std::string, std::vector<unsigned char>> getUserLogoutPacket(const std::string& userNicknameHash);
    };
}