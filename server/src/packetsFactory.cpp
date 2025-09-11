#include "packetsFactory.h"
#include "call.h"

#include "json.hpp"

std::string PacketsFactory::getFriendInfoSuccessPacket(const CryptoPP::RSA::PublicKey& publicKey, const std::string& nicknameHash) {
    nlohmann::json jsonObject;
    jsonObject[NICKNAME_HASH] = nicknameHash;
    jsonObject[PUBLIC_KEY] = crypto::serializePublicKey(publicKey);

    return jsonObject.dump();
}