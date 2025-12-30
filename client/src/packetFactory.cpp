#include "packetFactory.h"
#include "jsonTypes.h"

#include "json.hpp"

using namespace utilities;

using namespace calls
{
    namespace {
        std::vector<unsigned char> toBytes(const std::string& value) {
            return std::vector<unsigned char>(value.begin(), value.end());
        }
    }

    std::pair<std::string, std::vector<unsigned char>> PacketFactory::getAuthorizationPacket(const std::string& myNickname, const CryptoPP::RSA::PublicKey& myPublicKey) {
        std::string uuid = crypto::generateUUID();

        nlohmann::json jsonObject;
        jsonObject[UUID] = uuid;
        jsonObject[NICKNAME_HASH_SENDER] = crypto::calculateHash(myNickname);
        jsonObject[PUBLIC_KEY_SENDER] = crypto::serializePublicKey(myPublicKey);

        return std::make_pair(uuid, toBytes(jsonObject.dump()));
    }

    std::pair<std::string, std::vector<unsigned char>> PacketFactory::getLogoutPacket(const std::string& myNickname, bool needConfirmation) {
        std::string uuid = crypto::generateUUID();
        nlohmann::json jsonObject;
        jsonObject[UUID] = uuid;
        jsonObject[NICKNAME_HASH_SENDER] = crypto::calculateHash(myNickname);
        jsonObject[NEED_CONFIRMATION] = needConfirmation;
        return std::make_pair(uuid, toBytes(jsonObject.dump()));
    }

    std::pair<std::string, std::vector<unsigned char>> PacketFactory::getRequestFriendInfoPacket(const std::string& myNickname, const std::string& estimatedFriendNickname) {
        std::string uuid = crypto::generateUUID();

        nlohmann::json jsonObject;
        jsonObject[UUID] = uuid;
        jsonObject[NICKNAME_HASH] = crypto::calculateHash(estimatedFriendNickname);
        jsonObject[NICKNAME_HASH_SENDER] = crypto::calculateHash(myNickname);

        return std::make_pair(uuid, toBytes(jsonObject.dump()));
    }




    std::pair<std::string, std::vector<unsigned char>> PacketFactory::getStartCallingPacket(const std::string& myNickname, const CryptoPP::RSA::PublicKey& myPublicKey, const std::string& friendNickname, const CryptoPP::RSA::PublicKey& friendPublicKey, const CryptoPP::SecByteBlock& callKey) {
        CryptoPP::SecByteBlock thisPacketKey;
        crypto::generateAESKey(thisPacketKey);
        std::string uuid = crypto::generateUUID();

        nlohmann::json jsonObject;
        jsonObject[UUID] = uuid;
        jsonObject[PACKET_KEY] = crypto::RSAEncryptAESKey(friendPublicKey, thisPacketKey);
        jsonObject[NICKNAME] = crypto::AESEncrypt(thisPacketKey, myNickname);
        jsonObject[NICKNAME_HASH_SENDER] = crypto::calculateHash(myNickname);
        jsonObject[NICKNAME_HASH_RECEIVER] = crypto::calculateHash(friendNickname);
        jsonObject[CALL_KEY] = crypto::RSAEncryptAESKey(friendPublicKey, callKey);
        jsonObject[PUBLIC_KEY_SENDER] = crypto::serializePublicKey(myPublicKey);

        return std::make_pair(uuid, toBytes(jsonObject.dump()));
    }

    std::pair<std::string, std::vector<unsigned char>> PacketFactory::getStopCallingPacket(const std::string& myNickname, const std::string& friendNickname, bool needConfirmation) {
        std::string uuid = crypto::generateUUID();

        nlohmann::json jsonObject;
        jsonObject[UUID] = uuid;
        jsonObject[NICKNAME_HASH_SENDER] = crypto::calculateHash(myNickname);
        jsonObject[NICKNAME_HASH_RECEIVER] = crypto::calculateHash(friendNickname);
        jsonObject[NEED_CONFIRMATION] = needConfirmation;

        return std::make_pair(uuid, toBytes(jsonObject.dump()));
    }

    std::pair<std::string, std::vector<unsigned char>> PacketFactory::getStartScreenSharingPacket(const std::string& myNickname, const std::string& friendNicknameHash) {
        std::string uuid = crypto::generateUUID();

        nlohmann::json jsonObject;
        jsonObject[UUID] = uuid;
        jsonObject[NICKNAME_HASH_SENDER] = crypto::calculateHash(myNickname);
        jsonObject[NICKNAME_HASH_RECEIVER] = friendNicknameHash;

        return std::make_pair(uuid, toBytes(jsonObject.dump()));
    }

    std::pair<std::string, std::vector<unsigned char>> PacketFactory::getStopScreenSharingPacket(const std::string& myNickname, const std::string& friendNicknameHash, bool needConfirmation) {
        std::string uuid = crypto::generateUUID();

        nlohmann::json jsonObject;
        jsonObject[UUID] = uuid;
        jsonObject[NICKNAME_HASH_SENDER] = crypto::calculateHash(myNickname);
        jsonObject[NICKNAME_HASH_RECEIVER] = friendNicknameHash;
        jsonObject[NEED_CONFIRMATION] = needConfirmation;

        return std::make_pair(uuid, toBytes(jsonObject.dump()));
    }

    std::pair<std::string, std::vector<unsigned char>> PacketFactory::getStartCameraSharingPacket(const std::string& myNickname, const std::string& friendNicknameHash) {
        return getStartScreenSharingPacket(myNickname, friendNicknameHash);
    }

    std::pair<std::string, std::vector<unsigned char>> PacketFactory::getStopCameraSharingPacket(const std::string& myNickname, const std::string& friendNicknameHash, bool needConfirmation) {
        return getStopScreenSharingPacket(myNickname, friendNicknameHash, needConfirmation);
    }

    std::pair<std::string, std::vector<unsigned char>> PacketFactory::getDeclineCallPacket(const std::string& myNickname, const std::string& friendNickname, bool needConfirmation) {
        std::string uuid = crypto::generateUUID();

        nlohmann::json jsonObject;
        jsonObject[UUID] = uuid;
        jsonObject[NICKNAME_HASH_SENDER] = crypto::calculateHash(myNickname);
        jsonObject[NICKNAME_HASH_RECEIVER] = crypto::calculateHash(friendNickname);
        jsonObject[NEED_CONFIRMATION] = needConfirmation;

        return std::make_pair(uuid, toBytes(jsonObject.dump()));
    }

    std::pair<std::string, std::vector<unsigned char>> PacketFactory::getAcceptCallPacket(const std::string& myNickname, const std::string& friendNicknameToAccept) {
        std::string uuid = crypto::generateUUID();

        nlohmann::json jsonObject;
        jsonObject[UUID] = uuid;
        jsonObject[NICKNAME_HASH_SENDER] = crypto::calculateHash(myNickname);
        jsonObject[NICKNAME_HASH_RECEIVER] = crypto::calculateHash(friendNicknameToAccept);

        return std::make_pair(uuid, toBytes(jsonObject.dump()));
    }

    std::pair<std::string, std::vector<unsigned char>> PacketFactory::getEndCallPacket(const std::string& myNickname, bool needConfirmation) {
        std::string uuid = crypto::generateUUID();

        nlohmann::json jsonObject;
        jsonObject[UUID] = uuid;
        jsonObject[NICKNAME_HASH_SENDER] = crypto::calculateHash(myNickname);
        jsonObject[NEED_CONFIRMATION] = needConfirmation;

        return std::make_pair(uuid, toBytes(jsonObject.dump()));
    }

    std::vector<unsigned char> PacketFactory::getConfirmationPacket(const std::string& myNickname, const std::string& nicknameHashTo, const std::string& uuid) {
        nlohmann::json jsonObject;
        jsonObject[UUID] = uuid;
        jsonObject[NICKNAME_HASH_SENDER] = crypto::calculateHash(myNickname);
        jsonObject[NICKNAME_HASH_RECEIVER] = nicknameHashTo;
        return toBytes(jsonObject.dump());
    }
}