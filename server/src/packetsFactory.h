#pragma once 

#include <string>
#include "crypto.h"
#include "json.hpp"
#include "jsonTypes.h"

static std::string getPacketWithUuid(const std::string& uuid) {
    nlohmann::json jsonObject;
    jsonObject[UUID] = uuid;
    return jsonObject.dump();
}

static std::string getPacketCallAcceptedOk(const std::string& uuid, const std::string& friendNicknameHash) {
    nlohmann::json jsonObject;
    jsonObject[UUID] = uuid;
    jsonObject[NICKNAME_HASH] = friendNicknameHash;
    return jsonObject.dump();
}

static std::string getPacketFriendInfoSuccess(const std::string& uuid, const CryptoPP::RSA::PublicKey& friendPublicKey, const std::string& friendNicknameHash) {
    nlohmann::json jsonObject;
    jsonObject[UUID] = uuid;
    jsonObject[NICKNAME_HASH] = friendNicknameHash;
    jsonObject[PUBLIC_KEY] = crypto::serializePublicKey(friendPublicKey);
    return jsonObject.dump();
}