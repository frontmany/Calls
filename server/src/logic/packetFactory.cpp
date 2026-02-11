#include "packetFactory.h"
#include "constants/jsonType.h"
#include "json.hpp"

using namespace server::utilities;
using namespace server::constant;

namespace server
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
    jsonObject[SENDER_NICKNAME_HASH] = "server";
    jsonObject[RECEIVER_NICKNAME_HASH] = receiverNicknameHash;

    return toBytes(jsonObject.dump());
}

std::vector<unsigned char> PacketFactory::getAuthorizationResultPacket(bool authorized, const std::string& uid, const std::string& receiverNicknameHash, std::optional<std::string> receiverToken, std::optional<std::string> encryptedNickname, std::optional<std::string> packetKey) {
    nlohmann::json jsonObject;

    jsonObject[UID] = uid;
    jsonObject[RESULT] = authorized;
    jsonObject[NICKNAME_HASH] = receiverNicknameHash;

    if (authorized) {
        jsonObject[TOKEN] = receiverToken.value();
        if (encryptedNickname.has_value())
            jsonObject[ENCRYPTED_NICKNAME] = encryptedNickname.value();
        if (packetKey.has_value())
            jsonObject[PACKET_KEY] = packetKey.value();
    }

    return toBytes(jsonObject.dump());
}

std::vector<unsigned char> PacketFactory::getReconnectionResultPacket(bool reconnectedSuccessfully, const std::string& uid, const std::string& receiverNicknameHash, const std::string& receiverToken, std::optional<bool> activeCall) {
    nlohmann::json jsonObject;

    jsonObject[UID] = uid;
    jsonObject[RESULT] = reconnectedSuccessfully;
    jsonObject[NICKNAME_HASH] = receiverNicknameHash;
    jsonObject[TOKEN] = receiverToken;

    if (reconnectedSuccessfully && activeCall.has_value())
        jsonObject[IS_ACTIVE_CALL] = activeCall.value();

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

std::pair<std::string, std::vector<unsigned char>> PacketFactory::getCallDeclinedPacket(const std::string& senderNicknameHash, const std::string& receiverNicknameHash) {
    nlohmann::json jsonObject;
    std::string uid = crypto::generateUID();

    jsonObject[UID] = uid;
    jsonObject[SENDER_NICKNAME_HASH] = senderNicknameHash;
    jsonObject[RECEIVER_NICKNAME_HASH] = receiverNicknameHash;

    return std::make_pair(uid, toBytes(jsonObject.dump()));
}
} // namespace server