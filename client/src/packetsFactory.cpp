#include "packetsFactory.h"
#include "call.h"

#include "json.hpp"

std::string PacketsFactory::getAuthorizationPacket(const CryptoPP::RSA::PublicKey& myPublicKey, const std::string& myNickname) {
    nlohmann::json jsonObject;
    jsonObject[NICKNAME_HASH] = crypto::calculateHash(myNickname);
    jsonObject[PUBLIC_KEY] = crypto::serializePublicKey(myPublicKey);

    return jsonObject.dump();
}

std::string PacketsFactory::getCreateCallPacket(const CryptoPP::RSA::PublicKey& myPublicKey, const std::string& myNickname, const Call& call) {
    CryptoPP::SecByteBlock thisPacketKey;
    crypto::generateAESKey(thisPacketKey);

    nlohmann::json jsonObject;
    jsonObject[NICKNAME] = crypto::AESEncrypt(thisPacketKey, myNickname);
    jsonObject[NICKNAME_HASH] = crypto::calculateHash(call.getFriendNickname());
    jsonObject[CALL_KEY] = crypto::RSAEncryptAESKey(call.getFriendPublicKey(), call.getCallKey());
    jsonObject[PACKET_KEY] = crypto::RSAEncryptAESKey(call.getFriendPublicKey(), thisPacketKey);
    jsonObject[PUBLIC_KEY] = crypto::serializePublicKey(myPublicKey);

    return jsonObject.dump();
}

std::string PacketsFactory::getRequestFriendInfoPacket(const std::string& friendNickname) {
    nlohmann::json jsonObject;
    jsonObject[NICKNAME_HASH] = crypto::calculateHash(friendNickname);

    return jsonObject.dump();
}

std::string PacketsFactory::getDeclineCallPacket(const std::string& friendNickname) {
    nlohmann::json jsonObject;
    jsonObject[NICKNAME_HASH] = crypto::calculateHash(friendNickname);
    return jsonObject.dump();
}

std::string PacketsFactory::getAcceptCallPacket(const std::string& friendNickname) {
    nlohmann::json jsonObject;
    jsonObject[NICKNAME_HASH] = crypto::calculateHash(friendNickname);
    return jsonObject.dump();
}