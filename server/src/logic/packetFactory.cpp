#include "packetFactory.h"
#include "constants/jsonType.h"
#include <nlohmann/json.hpp>

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

using namespace server::utilities;
using namespace server::constant;

namespace server
{

namespace
{
    std::vector<unsigned char> toBytes(const std::string& value) {
        return std::vector<unsigned char>(value.begin(), value.end());
    }

    std::string utcTimestampIso8601() {
        using namespace std::chrono;
        const auto now = system_clock::now();
        const auto time = system_clock::to_time_t(now);
        const auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;
        std::ostringstream oss;
        oss << std::put_time(std::gmtime(&time), "%Y-%m-%dT%H:%M:%S")
            << '.' << std::setfill('0') << std::setw(3) << ms.count() << 'Z';
        return oss.str();
    }
}

std::vector<unsigned char> PacketFactory::getConfirmationPacket(const std::string& uid, const std::string& receiverNicknameHash) {
    nlohmann::json jsonObject;

    jsonObject[UID] = uid;
    jsonObject[SENDER_NICKNAME_HASH] = "server";
    jsonObject[RECEIVER_NICKNAME_HASH] = receiverNicknameHash;

    return toBytes(jsonObject.dump());
}

std::vector<unsigned char> PacketFactory::getAuthorizationResultPacket(bool authorized, const std::string& uid, const std::string& receiverNicknameHash, std::optional<std::string> receiverToken, std::optional<std::string> encryptedNickname, std::optional<std::string> packetKey) {
    nlohmann::json jsonObject;

    jsonObject[UID] = uid;
    jsonObject[RESULT] = authorized;
    jsonObject[NICKNAME_HASH] = receiverNicknameHash;

    if (authorized) {
        jsonObject[TOKEN] = receiverToken.value();
        if (encryptedNickname.has_value())
            jsonObject[ENCRYPTED_NICKNAME] = encryptedNickname.value();
        if (packetKey.has_value())
            jsonObject[PACKET_KEY] = packetKey.value();
    }

    return toBytes(jsonObject.dump());
}

std::vector<unsigned char> PacketFactory::getReconnectionResultPacket(
    bool reconnectedSuccessfully,
    const std::string& uid,
    const std::string& receiverNicknameHash,
    const std::string& receiverToken,
    std::optional<bool> activeCall,
    const std::string& callPartnerNicknameHash,
    std::optional<bool> isInMeeting,
    std::optional<std::string> meetingRosterJson)
{
    nlohmann::json jsonObject;

    jsonObject[UID] = uid;
    jsonObject[RESULT] = reconnectedSuccessfully;
    jsonObject[NICKNAME_HASH] = receiverNicknameHash;
    jsonObject[TOKEN] = receiverToken;

    if (reconnectedSuccessfully && activeCall.has_value()) {
        jsonObject[IS_ACTIVE_CALL] = activeCall.value();
        if (activeCall.value() && !callPartnerNicknameHash.empty())
            jsonObject[CALL_PARTNER_NICKNAME_HASH] = callPartnerNicknameHash;
    }

    if (reconnectedSuccessfully && isInMeeting.has_value()) {
        jsonObject[IS_IN_MEETING] = isInMeeting.value();
    }

    if (reconnectedSuccessfully && meetingRosterJson.has_value()) {
        try {
            jsonObject[MEETING_ROSTER] = nlohmann::json::parse(meetingRosterJson.value());
        } catch (...) {
            // Ignore parse errors; treat as absent roster.
        }
    }

    return toBytes(jsonObject.dump());
}

std::vector<unsigned char> PacketFactory::getUserInfoResultPacket(bool userInfoFound, const std::string& uid, const std::string& userNicknameHash, std::optional<CryptoPP::RSA::PublicKey> userPublicKey, std::optional<std::string> encryptedNickname, std::optional<std::string> packetKey) {
    nlohmann::json jsonObject;

    jsonObject[UID] = uid;
    jsonObject[RESULT] = userInfoFound;
    jsonObject[NICKNAME_HASH] = userNicknameHash;

    if (userInfoFound && userPublicKey.has_value())
        jsonObject[PUBLIC_KEY] = crypto::serializePublicKey(userPublicKey.value());
    if (userInfoFound && encryptedNickname.has_value())
        jsonObject[ENCRYPTED_NICKNAME] = encryptedNickname.value();
    if (userInfoFound && packetKey.has_value())
        jsonObject[PACKET_KEY] = packetKey.value();

    return toBytes(jsonObject.dump());
}

std::pair<std::string, std::vector<unsigned char>> PacketFactory::getConnectionDownWithUserPacket(const std::string& userNicknameHash) {
    nlohmann::json jsonObject;
    std::string uid = crypto::generateUID();

    jsonObject[UID] = uid;
    jsonObject[NICKNAME_HASH] = userNicknameHash;

    return std::make_pair(uid, toBytes(jsonObject.dump()));
}

std::pair<std::string, std::vector<unsigned char>> PacketFactory::getConnectionRestoredWithUserPacket(const std::string& userNicknameHash) {
    nlohmann::json jsonObject;
    std::string uid = crypto::generateUID();

    jsonObject[UID] = uid;
    jsonObject[NICKNAME_HASH] = userNicknameHash;

    return std::make_pair(uid, toBytes(jsonObject.dump()));
}

std::pair<std::string, std::vector<unsigned char>> PacketFactory::getUserLogoutPacket(const std::string& userNicknameHash) {
    nlohmann::json jsonObject;
    std::string uid = crypto::generateUID();

    jsonObject[UID] = uid;
    jsonObject[NICKNAME_HASH] = userNicknameHash;

    return std::make_pair(uid, toBytes(jsonObject.dump()));
}

std::vector<unsigned char> PacketFactory::getCallEndPacket(const std::string& senderNicknameHash) {
    nlohmann::json jsonObject;
    std::string uid = crypto::generateUID();

    jsonObject[UID] = uid;
    jsonObject[SENDER_NICKNAME_HASH] = senderNicknameHash;

    return toBytes(jsonObject.dump());
}

std::vector<unsigned char> PacketFactory::getMeetingCreateResultPacket(
    bool success,
    const std::string& meetingId,
    std::optional<std::string> encryptedMeetingKey,
    std::optional<std::string> packetKey)
{
    nlohmann::json jsonObject;
    jsonObject[UID] = crypto::generateUID();
    jsonObject[RESULT] = success;
    if (success) {
        jsonObject[MEETING_ID] = meetingId;
        if (encryptedMeetingKey.has_value()) {
            jsonObject[ENCRYPTED_MEETING_KEY] = encryptedMeetingKey.value();
        }
        if (packetKey.has_value()) {
            jsonObject[PACKET_KEY] = packetKey.value();
        }
    }
    return toBytes(jsonObject.dump());
}

std::vector<unsigned char> PacketFactory::getMeetingInfoResultPacket(bool found, const std::string& ownerPublicKey)
{
    nlohmann::json jsonObject;
    jsonObject[UID] = crypto::generateUID();
    jsonObject[RESULT] = found;
    if (found) {
        jsonObject[PUBLIC_KEY] = ownerPublicKey;
    }
    return toBytes(jsonObject.dump());
}

std::vector<unsigned char> PacketFactory::getMeetingEndedPacket()
{
    nlohmann::json jsonObject;
    jsonObject[UID] = crypto::generateUID();
    return toBytes(jsonObject.dump());
}

std::vector<unsigned char> PacketFactory::getMeetingParticipantJoinedPacket(const std::string& encryptedNickname, const std::string& serializedPublicKey)
{
    nlohmann::json jsonObject;
    jsonObject[UID] = crypto::generateUID();
    jsonObject[ENCRYPTED_NICKNAME] = encryptedNickname;
    if (!serializedPublicKey.empty()) {
        jsonObject[PUBLIC_KEY] = serializedPublicKey;
    }
    return toBytes(jsonObject.dump());
}

std::vector<unsigned char> PacketFactory::getMeetingParticipantLeftPacket(const std::string& nicknameHash)
{
    nlohmann::json jsonObject;
    jsonObject[UID] = crypto::generateUID();
    jsonObject[NICKNAME_HASH] = nicknameHash;
    return toBytes(jsonObject.dump());
}

std::vector<unsigned char> PacketFactory::getMeetingJoinRejectedPacket(const std::string& reason)
{
    nlohmann::json jsonObject;
    jsonObject[UID] = crypto::generateUID();
    jsonObject[REASON] = reason;
    return toBytes(jsonObject.dump());
}

std::vector<unsigned char> PacketFactory::getMetricsResultPacket(double cpuUsagePercent, uint64_t memoryUsedBytes, uint64_t memoryAvailableBytes, size_t activeUsers) {
    nlohmann::json jsonObject;

    jsonObject[CPU_USAGE] = cpuUsagePercent;
    jsonObject[MEMORY_USED] = memoryUsedBytes;
    jsonObject[MEMORY_AVAILABLE] = memoryAvailableBytes;
    jsonObject[ACTIVE_USERS] = activeUsers;
    jsonObject[RECORDED_AT] = utcTimestampIso8601();

    return toBytes(jsonObject.dump());
}

std::vector<unsigned char> PacketFactory::getMediaSharingBeginPacket(const std::string& senderNicknameHash)
{
    nlohmann::json jsonObject;
    jsonObject[UID] = crypto::generateUID();
    jsonObject[SENDER_NICKNAME_HASH] = senderNicknameHash;
    return toBytes(jsonObject.dump());
}

std::vector<unsigned char> PacketFactory::getMediaSharingEndPacket(const std::string& senderNicknameHash)
{
    nlohmann::json jsonObject;
    jsonObject[UID] = crypto::generateUID();
    jsonObject[SENDER_NICKNAME_HASH] = senderNicknameHash;
    return toBytes(jsonObject.dump());
}
} // namespace server