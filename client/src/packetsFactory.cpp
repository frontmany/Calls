#include "packetsFactory.h"
#include "call.h"

#include "json.hpp"

using namespace calls;

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
    jsonObject[NICKNAME_HASH] = call.getFriendNicknameHash();
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

std::string PacketsFactory::getCallingEndPacket(const std::string& myNickname, const std::string& friendNicknameHash) {
    nlohmann::json jsonObject;
    jsonObject[NICKNAME_HASH] = crypto::calculateHash(myNickname);
    jsonObject[NICKNAME_HASH_TO] = friendNicknameHash;

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

// new 
std::string PacketsFactory::getCreateGroupCallPacket(const std::string& nicknameHash, const std::string& groupCallName) {
    nlohmann::json jsonObject;
    jsonObject[NICKNAME_HASH] = crypto::calculateHash(nicknameHash);
    jsonObject[GROUP_CALL_NAME_HASH] = crypto::calculateHash(groupCallName);
    return jsonObject.dump();
}

std::string PacketsFactory::getJoinAllowedPacket(const CryptoPP::SecByteBlock& groupCallKey,
    const std::unordered_map<std::string, CryptoPP::RSA::PublicKey>& participants,
    const std::string& groupCallName,
    const std::string& nicknameTo,
    const CryptoPP::RSA::PublicKey& publicKeyTo) {

    CryptoPP::SecByteBlock thisPacketKey;
    crypto::generateAESKey(thisPacketKey);

    nlohmann::json jsonObject;

    nlohmann::json participantsArray = nlohmann::json::array();
    for (auto& [nickname, publicKey] : participants) {
        std::string publicKeyStr = crypto::serializePublicKey(publicKey);

        participantsArray.push_back({
            {NICKNAME, nickname},
            {PUBLIC_KEY, publicKeyStr}
        });
    }

    jsonObject[PARTICIPANTS_ARRAY] = participantsArray;
    jsonObject[NICKNAME_HASH_TO] = crypto::calculateHash(nicknameTo);
    jsonObject[GROUP_CALL_NAME] = crypto::AESEncrypt(thisPacketKey, groupCallName);
    jsonObject[GROUP_CALL_KEY] = crypto::RSAEncryptAESKey(publicKeyTo, groupCallKey);
    jsonObject[PACKET_KEY] = crypto::RSAEncryptAESKey(publicKeyTo, thisPacketKey);

    return jsonObject.dump();
}

std::string PacketsFactory::getGroupCallNewParticipantPacket(const std::string& nicknameTo, const CryptoPP::RSA::PublicKey& publicKeyTo, const std::string newParticipantNickname) {
    CryptoPP::SecByteBlock thisPacketKey;
    crypto::generateAESKey(thisPacketKey);
    
    nlohmann::json jsonObject;
    jsonObject[NICKNAME_HASH_TO] = crypto::calculateHash(nicknameTo);
    jsonObject[GROUP_CALL_NAME] = crypto::AESEncrypt(thisPacketKey, newParticipantNickname);
    jsonObject[PACKET_KEY] = crypto::RSAEncryptAESKey(publicKeyTo, thisPacketKey);
    
    return jsonObject.dump();
}