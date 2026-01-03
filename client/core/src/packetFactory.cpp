#include "packetFactory.h"
#include "jsonType.h"

#include "json.hpp"

using namespace utilities;

namespace callifornia
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

        nlohmann::json createUserToUserPacket(const std::string& uid, const std::string& senderNickname, const std::string& receiverNickname) {
            nlohmann::json jsonObject = createBasePacket(uid, senderNickname);
            jsonObject[RECEIVER_NICKNAME_HASH] = crypto::calculateHash(receiverNickname);
            return jsonObject;
        }

        std::pair<std::string, std::vector<unsigned char>> createPacketWithUid(const std::string& senderNickname, const std::string& receiverNickname = "") {
            std::string uid = crypto::generateUID();
            nlohmann::json jsonObject = receiverNickname.empty() 
                ? createBasePacket(uid, senderNickname)
                : createUserToUserPacket(uid, senderNickname, receiverNickname);
            return std::make_pair(uid, toBytes(jsonObject.dump()));
        }
    }

    // packets for server

    std::pair<std::string, std::vector<unsigned char>> PacketFactory::getAuthorizationPacket(const std::string& myNickname, const CryptoPP::RSA::PublicKey& myPublicKey) {
        std::string uid = crypto::generateUID();
        nlohmann::json jsonObject = createBasePacket(uid, myNickname);
        jsonObject[PUBLIC_KEY] = crypto::serializePublicKey(myPublicKey);
        return std::make_pair(uid, toBytes(jsonObject.dump()));
    }

    std::pair<std::string, std::vector<unsigned char>> PacketFactory::getLogoutPacket(const std::string& myNickname) {
        return createPacketWithUid(myNickname);
    }

    std::pair<std::string, std::vector<unsigned char>> PacketFactory::getReconnectPacket(const std::string& myNickname, const std::string& myToken) {
        std::string uid = crypto::generateUID();
        nlohmann::json jsonObject = createBasePacket(uid, myNickname);
        jsonObject[TOKEN] = myToken;
        return std::make_pair(uid, toBytes(jsonObject.dump()));
    }

    std::pair<std::string, std::vector<unsigned char>> PacketFactory::getRequestUserInfoPacket(const std::string& myNickname, const std::string& userNickname) {
        std::string uid = crypto::generateUID();
        nlohmann::json jsonObject = createBasePacket(uid, myNickname);
        jsonObject[NICKNAME_HASH] = crypto::calculateHash(userNickname);
        return std::make_pair(uid, toBytes(jsonObject.dump()));
    }

    // packets for other user

    std::vector<unsigned char> PacketFactory::getConfirmationPacket(const std::string& myNicknameHash, const std::string& userNicknameHash, const std::string& uid) {
        nlohmann::json jsonObject;

        jsonObject[UID] = uid;
        jsonObject[SENDER_NICKNAME_HASH] = myNicknameHash;
        jsonObject[RECEIVER_NICKNAME_HASH] = userNicknameHash;

        return toBytes(jsonObject.dump());
    }

    std::pair<std::string, std::vector<unsigned char>> PacketFactory::getStartOutgoingCallPacket(const std::string& myNickname, const std::string& userNickname, const CryptoPP::RSA::PublicKey& myPublicKey, const CryptoPP::RSA::PublicKey& userPublicKey, const CryptoPP::SecByteBlock& callKey) {
        CryptoPP::SecByteBlock packetKey;
        crypto::generateAESKey(packetKey);

        std::string uid = crypto::generateUID();

        nlohmann::json jsonObject = createUserToUserPacket(uid, myNickname, userNickname);
        jsonObject[ENCRYPTED_CALL_KEY] = crypto::RSAEncryptAESKey(userPublicKey, callKey);
        jsonObject[SENDER_PUBLIC_KEY] = crypto::serializePublicKey(myPublicKey);
        jsonObject[SENDER_ENCRYPTED_NICKNAME] = crypto::AESEncrypt(packetKey, myNickname);
        jsonObject[PACKET_KEY] = crypto::RSAEncryptAESKey(userPublicKey, packetKey);
        return std::make_pair(uid, toBytes(jsonObject.dump()));
    }

    std::pair<std::string, std::vector<unsigned char>> PacketFactory::getStopOutgoingCallPacket(const std::string& myNickname, const std::string& userNickname) {
        return createPacketWithUid(myNickname, userNickname);
    }

    std::pair<std::string, std::vector<unsigned char>> PacketFactory::getAcceptCallPacket(const std::string& myNickname, const std::string& userNickname, const CryptoPP::RSA::PublicKey& myPublicKey, const CryptoPP::RSA::PublicKey& userPublicKey, const CryptoPP::SecByteBlock& callKey) {
        CryptoPP::SecByteBlock packetKey;
        crypto::generateAESKey(packetKey);
        std::string uid = crypto::generateUID();
        nlohmann::json jsonObject = createUserToUserPacket(uid, myNickname, userNickname);
        jsonObject[ENCRYPTED_CALL_KEY] = crypto::RSAEncryptAESKey(userPublicKey, callKey);
        jsonObject[SENDER_PUBLIC_KEY] = crypto::serializePublicKey(myPublicKey);
        jsonObject[SENDER_ENCRYPTED_NICKNAME] = crypto::AESEncrypt(packetKey, myNickname);
        jsonObject[PACKET_KEY] = crypto::RSAEncryptAESKey(userPublicKey, packetKey);
        return std::make_pair(uid, toBytes(jsonObject.dump()));
    }

    std::pair<std::string, std::vector<unsigned char>> PacketFactory::getDeclineCallPacket(const std::string& myNickname, const std::string& userNickname) {
        return createPacketWithUid(myNickname, userNickname);
    }

    std::pair<std::string, std::vector<unsigned char>> PacketFactory::getEndCallPacket(const std::string& myNickname, const std::string& userNickname) {
        return createPacketWithUid(myNickname, userNickname);
    }

    std::pair<std::string, std::vector<unsigned char>> PacketFactory::getStartScreenSharingPacket(const std::string& myNickname, const std::string& userNickname) {
        return createPacketWithUid(myNickname, userNickname);
    }

    std::pair<std::string, std::vector<unsigned char>> PacketFactory::getStopScreenSharingPacket(const std::string& myNickname, const std::string& userNickname) {
        return createPacketWithUid(myNickname, userNickname);
    }

    std::pair<std::string, std::vector<unsigned char>> PacketFactory::getStartCameraSharingPacket(const std::string& myNickname, const std::string& userNickname) {
        return createPacketWithUid(myNickname, userNickname);
    }

    std::pair<std::string, std::vector<unsigned char>> PacketFactory::getStopCameraSharingPacket(const std::string& myNickname, const std::string& userNickname) {
        return createPacketWithUid(myNickname, userNickname);
    }
}