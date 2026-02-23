#include "packetFactory.h"
#include "constants/jsonType.h"

#include "json.hpp"

using namespace core::constant;
using namespace core::utilities::crypto;

namespace core::logic
{
    namespace
    {
        std::vector<unsigned char> toBytes(const std::string& value) {
            return std::vector<unsigned char>(value.begin(), value.end());
        }

        nlohmann::json createBasePacket(const std::string& uid, const std::string& senderNickname) {
            nlohmann::json jsonObject;
            jsonObject[UID] = uid;
            jsonObject[SENDER_NICKNAME_HASH] = calculateHash(senderNickname);
            return jsonObject;
        }

        nlohmann::json createUserToUserBasePacket(const std::string& uid, const std::string& senderNickname, const std::string& receiverNickname) {
            nlohmann::json jsonObject = createBasePacket(uid, senderNickname);
            jsonObject[RECEIVER_NICKNAME_HASH] = calculateHash(receiverNickname);
            return jsonObject;
        }
    }

    std::vector<unsigned char> PacketFactory::getNicknamePacket(const std::string& myNickname) {
        std::string uid = generateUID();
        nlohmann::json jsonObject = createBasePacket(uid, myNickname);
        return toBytes(jsonObject.dump());
    }

    std::vector<unsigned char> PacketFactory::getTwoNicknamesPacket(const std::string& myNickname, const std::string& userNickname) {
        std::string uid = generateUID();
        nlohmann::json jsonObject = createUserToUserBasePacket(uid, myNickname, userNickname);
        return toBytes(jsonObject.dump());
    }

    std::vector<unsigned char> PacketFactory::getCallEndPacketWithHashes(const std::string& senderNicknameHash, const std::string& receiverNicknameHash) {
        std::string uid = generateUID();
        nlohmann::json jsonObject;
        jsonObject[UID] = uid;
        jsonObject[SENDER_NICKNAME_HASH] = senderNicknameHash;
        jsonObject[RECEIVER_NICKNAME_HASH] = receiverNicknameHash;
        return toBytes(jsonObject.dump());
    }

    std::vector<unsigned char> PacketFactory::getAuthorizationPacket(const std::string& myNickname, const CryptoPP::RSA::PublicKey& myPublicKey, uint16_t myUdpPort) {
        CryptoPP::SecByteBlock packetKey;
        generateAESKey(packetKey);
        std::string uid = generateUID();

        nlohmann::json jsonObject = createBasePacket(uid, myNickname);
        jsonObject[ENCRYPTED_NICKNAME] = AESEncrypt(packetKey, myNickname);
        jsonObject[PUBLIC_KEY] = serializePublicKey(myPublicKey);
        jsonObject[UDP_PORT] = myUdpPort;
        jsonObject[PACKET_KEY] = RSAEncryptAESKey(myPublicKey, packetKey);

        return toBytes(jsonObject.dump());
    }

    std::vector<unsigned char> PacketFactory::getReconnectPacket(const std::string& myNickname, const std::string& myToken, uint16_t myUdpPort) {
        std::string uid = generateUID();
        nlohmann::json jsonObject = createBasePacket(uid, myNickname);
        jsonObject[TOKEN] = myToken;
        jsonObject[UDP_PORT] = myUdpPort;
        return toBytes(jsonObject.dump());
    }

    std::vector<unsigned char> PacketFactory::getRequestUserInfoPacket(const std::string& myNickname, const CryptoPP::RSA::PublicKey& myPublicKey, const std::string& userNickname) {
        CryptoPP::SecByteBlock packetKey;
        generateAESKey(packetKey);
        std::string uid = generateUID();

        nlohmann::json jsonObject = createBasePacket(uid, myNickname);
        jsonObject[ENCRYPTED_NICKNAME] = AESEncrypt(packetKey, userNickname);
        jsonObject[NICKNAME_HASH] = calculateHash(userNickname);
        jsonObject[PACKET_KEY] = RSAEncryptAESKey(myPublicKey, packetKey);
        return toBytes(jsonObject.dump());
    }

    std::vector<unsigned char> PacketFactory::getCallPacketWithKeys(
        const std::string& myNickname,
        const std::string& userNickname,
        const CryptoPP::RSA::PublicKey& myPublicKey,
        const CryptoPP::RSA::PublicKey& userPublicKey,
        const CryptoPP::SecByteBlock& callKey) 
    {
        CryptoPP::SecByteBlock packetKey;
        generateAESKey(packetKey);
        std::string uid = generateUID();

        nlohmann::json jsonObject = createUserToUserBasePacket(uid, myNickname, userNickname);
        jsonObject[ENCRYPTED_CALL_KEY] = RSAEncryptAESKey(userPublicKey, callKey);
        jsonObject[SENDER_PUBLIC_KEY] = serializePublicKey(myPublicKey);
        jsonObject[SENDER_ENCRYPTED_NICKNAME] = AESEncrypt(packetKey, myNickname);
        jsonObject[PACKET_KEY] = RSAEncryptAESKey(userPublicKey, packetKey);

        return toBytes(jsonObject.dump());
    }

    std::vector<unsigned char> PacketFactory::getGetMetricsPacket(const std::string& myNickname) {
        std::string uid = generateUID();
        nlohmann::json jsonObject = createBasePacket(uid, myNickname);
        return toBytes(jsonObject.dump());
    }
}