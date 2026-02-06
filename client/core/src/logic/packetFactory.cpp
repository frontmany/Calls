#include "packetFactory.h"
#include "utilities/jsonType.h"

#include "json.hpp"

namespace core
{
    namespace
    {
        std::vector<unsigned char> toBytes(const std::string& value) {
            return std::vector<unsigned char>(value.begin(), value.end());
        }

        nlohmann::json createBasePacket(const std::string& uid, const std::string& senderNickname) {
            nlohmann::json jsonObject;
            jsonObject[UID] = uid;
            jsonObject[SENDER_NICKNAME_HASH] = crypto::calculateHash(senderNickname);
            return jsonObject;
        }

        nlohmann::json createUserToUserBasePacket(const std::string& uid, const std::string& senderNickname, const std::string& receiverNickname) {
            nlohmann::json jsonObject = createBasePacket(uid, senderNickname);
            jsonObject[RECEIVER_NICKNAME_HASH] = crypto::calculateHash(receiverNickname);
            return jsonObject;
        }
    }

    // Универсальные методы

    std::vector<unsigned char> PacketFactory::getNicknamePacket(const std::string& myNickname) {
        std::string uid = crypto::generateUID();
        nlohmann::json jsonObject = createBasePacket(uid, myNickname);
        return toBytes(jsonObject.dump());
    }

    std::vector<unsigned char> PacketFactory::getTwoNicknamesPacket(const std::string& myNickname, const std::string& userNickname) {
        std::string uid = crypto::generateUID();
        nlohmann::json jsonObject = createUserToUserBasePacket(uid, myNickname, userNickname);
        return toBytes(jsonObject.dump());
    }

    // Пакеты с уникальными полями

    std::vector<unsigned char> PacketFactory::getAuthorizationPacket(const std::string& myNickname, const CryptoPP::RSA::PublicKey& myPublicKey, uint16_t udpPort) {
        std::string uid = crypto::generateUID();
        nlohmann::json jsonObject = createBasePacket(uid, myNickname);
        jsonObject[PUBLIC_KEY] = crypto::serializePublicKey(myPublicKey);
        jsonObject[UDP_PORT] = udpPort;
        return toBytes(jsonObject.dump());
    }

    std::vector<unsigned char> PacketFactory::getReconnectPacket(const std::string& myNickname, const std::string& myToken, uint16_t udpPort) {
        std::string uid = crypto::generateUID();
        nlohmann::json jsonObject = createBasePacket(uid, myNickname);
        jsonObject[TOKEN] = myToken;
        jsonObject[UDP_PORT] = udpPort;
        return toBytes(jsonObject.dump());
    }

    std::vector<unsigned char> PacketFactory::getRequestUserInfoPacket(const std::string& myNickname, const std::string& userNickname) {
        std::string uid = crypto::generateUID();
        nlohmann::json jsonObject = createBasePacket(uid, myNickname);
        jsonObject[NICKNAME_HASH] = crypto::calculateHash(userNickname);
        return toBytes(jsonObject.dump());
    }

    std::vector<unsigned char> PacketFactory::getCallPacketWithKeys(
        const std::string& myNickname,
        const std::string& userNickname,
        const CryptoPP::RSA::PublicKey& myPublicKey,
        const CryptoPP::RSA::PublicKey& userPublicKey,
        const CryptoPP::SecByteBlock& callKey
    ) {
        CryptoPP::SecByteBlock packetKey;
        crypto::generateAESKey(packetKey);
        std::string uid = crypto::generateUID();

        nlohmann::json jsonObject = createUserToUserBasePacket(uid, myNickname, userNickname);
        jsonObject[ENCRYPTED_CALL_KEY] = crypto::RSAEncryptAESKey(userPublicKey, callKey);
        jsonObject[SENDER_PUBLIC_KEY] = crypto::serializePublicKey(myPublicKey);
        jsonObject[SENDER_ENCRYPTED_NICKNAME] = crypto::AESEncrypt(packetKey, myNickname);
        jsonObject[PACKET_KEY] = crypto::RSAEncryptAESKey(userPublicKey, packetKey);

        return toBytes(jsonObject.dump());
    }
}