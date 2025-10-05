#include "packetsFactory.h"
#include "jsonTypes.h"

#include "json.hpp"

using namespace calls;

std::string PacketsFactory::getAuthorizationPacket(const std::string& myNickname, const CryptoPP::RSA::PublicKey& myPublicKey) {
    nlohmann::json jsonObject;
    jsonObject[NICKNAME_HASH_SENDER] = crypto::calculateHash(myNickname);
    jsonObject[PUBLIC_KEY_SENDER] = crypto::serializePublicKey(myPublicKey);

    return jsonObject.dump();
}

std::string PacketsFactory::getCreateCallPacket(const std::string& myNickname, const CryptoPP::RSA::PublicKey& myPublicKey, const std::string& friendNickname, const CryptoPP::RSA::PublicKey& friendPublicKey, const CryptoPP::SecByteBlock& callKey) {
    CryptoPP::SecByteBlock thisPacketKey;
    crypto::generateAESKey(thisPacketKey);

    nlohmann::json jsonObject;
    jsonObject[PACKET_KEY] = crypto::RSAEncryptAESKey(friendPublicKey, thisPacketKey);
    jsonObject[NICKNAME] = crypto::AESEncrypt(thisPacketKey, myNickname);
    jsonObject[NICKNAME_HASH_SENDER] = crypto::calculateHash(myNickname);
    jsonObject[NICKNAME_HASH_RECEIVER] = crypto::calculateHash(friendNickname);
    jsonObject[CALL_KEY] = crypto::RSAEncryptAESKey(friendPublicKey, callKey);
    jsonObject[PUBLIC_KEY_SENDER] = crypto::serializePublicKey(myPublicKey);

    return jsonObject.dump();
}

std::string PacketsFactory::getRequestFriendInfoPacket(const std::string& myNickname, const std::string& estimatedFriendNickname) {
    nlohmann::json jsonObject;
    jsonObject[NICKNAME_HASH] = crypto::calculateHash(estimatedFriendNickname);
    jsonObject[NICKNAME_HASH_SENDER] = crypto::calculateHash(myNickname);

    return jsonObject.dump();
}

std::string PacketsFactory::getCallingEndPacket(const std::string& myNickname, const std::string& friendNickname) {
    nlohmann::json jsonObject;
    jsonObject[NICKNAME_HASH_SENDER] = crypto::calculateHash(myNickname);
    jsonObject[NICKNAME_HASH_RECEIVER] = crypto::calculateHash(friendNickname);

    return jsonObject.dump();
}

std::string PacketsFactory::getDeclineCallPacket(const std::string& myNickname, const std::string& friendNickname) {
    nlohmann::json jsonObject;
    jsonObject[NICKNAME_HASH_SENDER] = crypto::calculateHash(myNickname);
    jsonObject[NICKNAME_HASH] = crypto::calculateHash(friendNickname);
    return jsonObject.dump();
}

std::string PacketsFactory::getDeclineAllCallsPacket(const std::string& myNickname, const std::vector<std::string>& nicknames) {
    nlohmann::json jsonObject;
    jsonObject[NICKNAME_HASH_SENDER] = crypto::calculateHash(myNickname);

    nlohmann::json nicknameHashesArray = nlohmann::json::array();

    for (const auto& nickname : nicknames) {
        nicknameHashesArray.push_back(crypto::calculateHash(nickname));
    }

    jsonObject[ARRAY_NICKNAME_HASHES] = nicknameHashesArray;
    return jsonObject.dump();
}

std::string PacketsFactory::getAcceptCallPacket(const std::string& myNickname, const std::string& friendNickname) {
    nlohmann::json jsonObject;
    jsonObject[NICKNAME_HASH_SENDER] = crypto::calculateHash(myNickname);
    jsonObject[NICKNAME_HASH] = crypto::calculateHash(friendNickname);
    return jsonObject.dump();
}

std::string PacketsFactory::getEndCallPacket(const std::string& myNickname) {
    nlohmann::json jsonObject;
    jsonObject[NICKNAME_HASH_SENDER] = crypto::calculateHash(myNickname);
    return jsonObject.dump();
}

std::string PacketsFactory::getLogoutPacket(const std::string& myNickname) {
    nlohmann::json jsonObject;
    jsonObject[NICKNAME_HASH_SENDER] = crypto::calculateHash(myNickname);
    return jsonObject.dump();
}