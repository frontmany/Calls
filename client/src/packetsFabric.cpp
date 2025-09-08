#include "packetsFabric.h"
#include "call.h"

#include "json.hpp"

std::string PacketsFabric::getAuthorizationPacket(const CryptoPP::RSA::PublicKey& myPublicKey, const std::string& myNickname) {
    nlohmann::json jsonObject;
    jsonObject[NICKNAME_HASH] = crypto::calculateHash(myNickname);
    jsonObject[PUBLIC_KEY] = crypto::serializePublicKey(myPublicKey);

    return jsonObject.dump();
}

std::string PacketsFabric::getCreateCallPacket(const CryptoPP::RSA::PublicKey& myPublicKey, const std::string& myNickname, const Call& call) {
    CryptoPP::SecByteBlock thisPacketKey;
    crypto::generateAESKey(thisPacketKey);

    auto callKeyOpt = call.getCallKey();
    if (!callKeyOpt) {
        return "";
    }

    nlohmann::json jsonObject;
    jsonObject[NICKNAME] = crypto::AESEncrypt(thisPacketKey, myNickname);
    jsonObject[CALL_KEY] = crypto::RSAEncryptAESKey(call.getFriendPublicKey(), *callKeyOpt);
    jsonObject[PACKET_KEY] = crypto::RSAEncryptAESKey(call.getFriendPublicKey(), thisPacketKey);
    jsonObject[PUBLIC_KEY] = crypto::serializePublicKey(myPublicKey);

    return jsonObject.dump();
}

std::string PacketsFabric::getRequestFriendInfoPacket(const std::string& friendNickname) {
    nlohmann::json jsonObject;
    jsonObject[NICKNAME_HASH] = crypto::calculateHash(friendNickname);

    return jsonObject.dump();
}