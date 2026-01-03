#include "packetFactory.h"

using namespace utilities;

namespace callifornia
{
namespace
{
    std::vector<unsigned char> toBytes(const std::string& value) {
        return std::vector<unsigned char>(value.begin(), value.end());
    }
}

std::vector<unsigned char> PacketFactory::getConfirmationPacket(const std::string& uid, const std::string& receiverNicknameHash) {
    nlohmann::json jsonObject;

    jsonObject[UID] = uid;
    jsonObject[NICKNAME_HASH] = receiverNicknameHash;

    return toBytes(jsonObject.dump());
}

std::vector<unsigned char> PacketFactory::getAuthorizationResultPacket(bool authorized, const std::string& uid, const std::string& receiverNicknameHash, std::optional<std::string> receiverToken) {
    nlohmann::json jsonObject;

    jsonObject[UID] = uid;
    jsonObject[RESULT] = authorized;
    jsonObject[NICKNAME_HASH] = receiverNicknameHash;

    if (authorized) 
        jsonObject[TOKEN] = receiverToken.value();

    return toBytes(jsonObject.dump());
}

std::vector<unsigned char> PacketFactory::getReconnectionResultPacket(bool reconnectedSuccessfully, const std::string& uid, const std::string& receiverNicknameHash, const std::string& receiverToken, std::optional<bool> activeCal) {
    nlohmann::json jsonObject;

    jsonObject[UID] = uid;
    jsonObject[RESULT] = reconnectedSuccessfully;
    jsonObject[NICKNAME_HASH] = receiverNicknameHash;
    jsonObject[TOKEN] = receiverToken;

    if (reconnectedSuccessfully)
        jsonObject[IS_ACTIVE_CALL] = activeCal.value();

    return toBytes(jsonObject.dump());
}

std::vector<unsigned char> PacketFactory::getUserInfoResultPacket(bool userInfoFound, const std::string& uid, const std::string& userNicknameHash, std::optional<CryptoPP::RSA::PublicKey> userPublicKey) {
    nlohmann::json jsonObject;

    jsonObject[UID] = uid;
    jsonObject[RESULT] = userInfoFound;
    jsonObject[NICKNAME_HASH] = userNicknameHash;

    if (userInfoFound && userPublicKey.has_value()) 
        jsonObject[PUBLIC_KEY] = crypto::serializePublicKey(userPublicKey.value());

    return toBytes(jsonObject.dump());
}

std::pair<std::string, std::vector<unsigned char>> PacketFactory::getConnectionDownWithUserPacket(const std::string& userNicknameHash) {
    nlohmann::json jsonObject;
    std::string uid = crypto::generateUID();

    jsonObject[UID] = uid;
    jsonObject[NICKNAME_HASH] = userNicknameHash;

    return std::make_pair(uid, toBytes(jsonObject.dump()));
}

std::pair<std::string, std::vector<unsigned char>> PacketFactory::getConnectionRestoredWithUserPacket(const std::string& userNicknameHash) {
    nlohmann::json jsonObject;
    std::string uid = crypto::generateUID();

    jsonObject[UID] = uid;
    jsonObject[NICKNAME_HASH] = userNicknameHash;

    return std::make_pair(uid, toBytes(jsonObject.dump()));
}

std::pair<std::string, std::vector<unsigned char>> PacketFactory::getUserLogoutPacket(const std::string& userNicknameHash) {
    nlohmann::json jsonObject;
    std::string uid = crypto::generateUID();

    jsonObject[UID] = uid;
    jsonObject[NICKNAME_HASH] = userNicknameHash;

    return std::make_pair(uid, toBytes(jsonObject.dump()));
}
} // namespace callifornia