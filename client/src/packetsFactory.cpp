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

std::pair<std::string, std::string> PacketsFactory::getCreateCallPacket(const std::string& myNickname, const CryptoPP::RSA::PublicKey& myPublicKey, const std::string& friendNickname, const CryptoPP::RSA::PublicKey& friendPublicKey, const CryptoPP::SecByteBlock& callKey) {
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

std::pair<std::string, std::string> PacketsFactory::getRequestFriendInfoPacket(const std::string& myNickname, const std::string& estimatedFriendNickname) {
    std::string uuid = crypto::generateUUID();
    
    nlohmann::json jsonObject;
    jsonObject[UUID] = uuid;
    jsonObject[NICKNAME_HASH] = crypto::calculateHash(estimatedFriendNickname);
    jsonObject[NICKNAME_HASH_SENDER] = crypto::calculateHash(myNickname);

    return std::make_pair(uuid, jsonObject.dump());
}

std::pair<std::string, std::string> PacketsFactory::getCallingEndPacket(const std::string& myNickname, const std::string& friendNickname) {
    std::string uuid = crypto::generateUUID();
    
    nlohmann::json jsonObject;
    jsonObject[UUID] = uuid;
    jsonObject[NICKNAME_HASH_SENDER] = crypto::calculateHash(myNickname);
    jsonObject[NICKNAME_HASH_RECEIVER] = crypto::calculateHash(friendNickname);

    return std::make_pair(uuid, jsonObject.dump());
}

std::pair<std::string, std::string> PacketsFactory::getDeclineCallPacket(const std::string& myNickname, const std::string& friendNickname) {
    std::string uuid = crypto::generateUUID();
    
    nlohmann::json jsonObject;
    jsonObject[UUID] = uuid;
    jsonObject[NICKNAME_HASH_SENDER] = crypto::calculateHash(myNickname);
    jsonObject[NICKNAME_HASH] = crypto::calculateHash(friendNickname);

    return std::make_pair(uuid, jsonObject.dump());
}

std::pair<std::string, std::string> PacketsFactory::getAcceptCallPacket(const std::string& myNickname, const std::string& friendNicknameToAccept, const std::vector<std::string>& nicknamesToDecline) {
    std::string uuid = crypto::generateUUID();
    
    nlohmann::json jsonObject;
    jsonObject[UUID] = uuid;
    jsonObject[NICKNAME_HASH_SENDER] = crypto::calculateHash(myNickname);
    jsonObject[NICKNAME_HASH] = crypto::calculateHash(friendNicknameToAccept);

    nlohmann::json nicknameHashesArray = nlohmann::json::array();
    for (const auto& nickname : nicknamesToDecline) {
        nicknameHashesArray.push_back(crypto::calculateHash(nickname));
    }

    jsonObject[ARRAY_NICKNAME_HASHES] = nicknameHashesArray;

    return std::make_pair(uuid, jsonObject.dump());
}

std::pair<std::string, std::string> PacketsFactory::getAcceptCallPacketAndEndCalling(const std::string& myNickname, const std::string& friendNicknameToAccept, const std::vector<std::string>& nicknamesToDecline, const std::string& friendNicknameToEndCalling) {
    std::string uuid = crypto::generateUUID();

    nlohmann::json jsonObject;
    jsonObject[UUID] = uuid;
    jsonObject[NICKNAME_HASH_SENDER] = crypto::calculateHash(myNickname);
    jsonObject[NICKNAME_HASH] = crypto::calculateHash(friendNicknameToAccept);

    nlohmann::json nicknameHashesArray = nlohmann::json::array();
    for (const auto& nickname : nicknamesToDecline) {
        nicknameHashesArray.push_back(crypto::calculateHash(nickname));
    }

    jsonObject[ARRAY_NICKNAME_HASHES] = nicknameHashesArray;
    jsonObject[NICKNAME_HASH_RECEIVER] = crypto::calculateHash(friendNicknameToEndCalling);

    return std::make_pair(uuid, jsonObject.dump());
}

std::pair<std::string, std::string> PacketsFactory::getEndCallPacket(const std::string& myNickname) {
    std::string uuid = crypto::generateUUID();
    
    nlohmann::json jsonObject;
    jsonObject[UUID] = uuid;
    jsonObject[NICKNAME_HASH_SENDER] = crypto::calculateHash(myNickname);

    return std::make_pair(uuid, jsonObject.dump());
}

std::pair<std::string, std::string> PacketsFactory::getLogoutPacket(const std::string& myNickname, const std::vector<std::string>& nicknamesToDecline, const std::string& friendNicknameToEndCalling) {
    std::string uuid = crypto::generateUUID();
    
    nlohmann::json jsonObject;
    jsonObject[UUID] = uuid;
    jsonObject[NICKNAME_HASH_SENDER] = crypto::calculateHash(myNickname);

    if (nicknamesToDecline.size() != 0) {
        nlohmann::json nicknameHashesArray = nlohmann::json::array();
        for (const auto& nickname : nicknamesToDecline) {
            nicknameHashesArray.push_back(crypto::calculateHash(nickname));
        }

        jsonObject[ARRAY_NICKNAME_HASHES] = nicknameHashesArray;
    }
    if (friendNicknameToEndCalling != "") {
        jsonObject[NICKNAME_HASH] = crypto::calculateHash(friendNicknameToEndCalling);
    }
    
    return std::make_pair(uuid, jsonObject.dump());
}