#include "packetsFactory.h"
#include "jsonTypes.h"

#include "json.hpp"

using namespace calls;

std::pair<std::string, std::string> PacketsFactory::getAuthorizationPacket(const std::string& myNickname, const CryptoPP::RSA::PublicKey& myPublicKey) {
    std::string uuid = crypto::generateUUID();
    
    nlohmann::json jsonObject;
    jsonObject[UUID] = uuid;
    jsonObject[NICKNAME_HASH_SENDER] = crypto::calculateHash(myNickname);
    jsonObject[PUBLIC_KEY_SENDER] = crypto::serializePublicKey(myPublicKey);

    return std::make_pair(uuid, jsonObject.dump());
}

std::pair<std::string, std::string> PacketsFactory::getLogoutPacket(const std::string& myNickname, bool needConfirmation) {
    std::string uuid = crypto::generateUUID();
    nlohmann::json jsonObject;
    jsonObject[UUID] = uuid;
    jsonObject[NICKNAME_HASH_SENDER] = crypto::calculateHash(myNickname);
    jsonObject[NEED_CONFIRMATION] = needConfirmation;
    return std::make_pair(uuid, jsonObject.dump());
}

std::pair<std::string, std::string> PacketsFactory::getRequestFriendInfoPacket(const std::string& myNickname, const std::string& estimatedFriendNickname) {
    std::string uuid = crypto::generateUUID();

    nlohmann::json jsonObject;
    jsonObject[UUID] = uuid;
    jsonObject[NICKNAME_HASH] = crypto::calculateHash(estimatedFriendNickname);
    jsonObject[NICKNAME_HASH_SENDER] = crypto::calculateHash(myNickname);

    return std::make_pair(uuid, jsonObject.dump());
}




std::pair<std::string, std::string> PacketsFactory::getStartCallingPacket(const std::string& myNickname, const CryptoPP::RSA::PublicKey& myPublicKey, const std::string& friendNickname, const CryptoPP::RSA::PublicKey& friendPublicKey, const CryptoPP::SecByteBlock& callKey) {
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

    return std::make_pair(uuid, jsonObject.dump());
}

std::pair<std::string, std::string> PacketsFactory::getStopCallingPacket(const std::string& myNickname, const std::string& friendNickname, bool needConfirmation) {
    std::string uuid = crypto::generateUUID();
    
    nlohmann::json jsonObject;
    jsonObject[UUID] = uuid;
    jsonObject[NICKNAME_HASH_SENDER] = crypto::calculateHash(myNickname);
    jsonObject[NICKNAME_HASH_RECEIVER] = crypto::calculateHash(friendNickname);
    jsonObject[NEED_CONFIRMATION] = needConfirmation;

    return std::make_pair(uuid, jsonObject.dump());
}

std::pair<std::string, std::string> PacketsFactory::getDeclineCallPacket(const std::string& myNickname, const std::string& friendNickname, bool needConfirmation) {
    std::string uuid = crypto::generateUUID();
    
    nlohmann::json jsonObject;
    jsonObject[UUID] = uuid;
    jsonObject[NICKNAME_HASH_SENDER] = crypto::calculateHash(myNickname);
    jsonObject[NICKNAME_HASH_RECEIVER] = crypto::calculateHash(friendNickname);
    jsonObject[NEED_CONFIRMATION] = needConfirmation;

    return std::make_pair(uuid, jsonObject.dump());
}

std::pair<std::string, std::string> PacketsFactory::getAcceptCallPacket(const std::string& myNickname, const std::string& friendNicknameToAccept) {
    std::string uuid = crypto::generateUUID();

    nlohmann::json jsonObject;
    jsonObject[UUID] = uuid;
    jsonObject[NICKNAME_HASH_SENDER] = crypto::calculateHash(myNickname);
    jsonObject[NICKNAME_HASH_RECEIVER] = crypto::calculateHash(friendNicknameToAccept);

    return std::make_pair(uuid, jsonObject.dump());
}

std::pair<std::string, std::string> PacketsFactory::getEndCallPacket(const std::string& myNickname, bool needConfirmation) {
    std::string uuid = crypto::generateUUID();
    
    nlohmann::json jsonObject;
    jsonObject[UUID] = uuid;
    jsonObject[NICKNAME_HASH_SENDER] = crypto::calculateHash(myNickname);
    jsonObject[NEED_CONFIRMATION] = needConfirmation;

    return std::make_pair(uuid, jsonObject.dump());
}

std::string PacketsFactory::getConfirmationPacket(const std::string& myNickname, const std::string& nicknameHashTo, const std::string& uuid) {
    nlohmann::json jsonObject;
    jsonObject[UUID] = uuid;
    jsonObject[NICKNAME_HASH_SENDER] = crypto::calculateHash(myNickname);
    jsonObject[NICKNAME_HASH_RECEIVER] = nicknameHashTo;
    return jsonObject.dump();
}