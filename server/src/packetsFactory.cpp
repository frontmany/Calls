#include "packetsFactory.h"
#include "call.h"

#include "json.hpp"

std::string PacketsFactory::getFriendInfoSuccessPacket(const CryptoPP::RSA::PublicKey& publicKey, const std::string& nicknameHash) {
    nlohmann::json jsonObject;
    jsonObject[NICKNAME_HASH] = nicknameHash;
    jsonObject[PUBLIC_KEY] = crypto::serializePublicKey(publicKey);

    return jsonObject.dump();
}

std::string PacketsFactory::getParticipantLeavePacket(const std::string& participantNicknameHash, const std::string& groupCallNameHash) {
    nlohmann::json jsonObject;
    jsonObject[NICKNAME_HASH] = participantNicknameHash;
    jsonObject[GROUP_CALL_NAME_HASH] = groupCallNameHash;

    return jsonObject.dump();
}

std::string PacketsFactory::getJoinRequestSuccessPacket(const CryptoPP::RSA::PublicKey& initiatorPublicKey, const std::string& initiatorNicknameHash) {
    nlohmann::json jsonObject;
    jsonObject[NICKNAME_HASH] = initiatorNicknameHash;
    jsonObject[PUBLIC_KEY] = crypto::serializePublicKey(initiatorPublicKey);

    return jsonObject.dump();
}