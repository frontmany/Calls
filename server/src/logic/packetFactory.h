#pragma once 

#include <string>
#include <vector>
#include <optional>
#include <cstddef>
#include <cstdint>
#include <nlohmann/json.hpp>

#include "utilities/crypto.h"

namespace server
{
    class PacketFactory {
    public:
        PacketFactory() = default;
    
        static std::vector<unsigned char> getConfirmationPacket(const std::string& uid, const std::string& receiverNicknameHash);
        static std::vector<unsigned char> getAuthorizationResultPacket(bool authorized, const std::string& uid, const std::string& receiverNicknameHash, std::optional<std::string> receiverToken = std::nullopt, std::optional<std::string> encryptedNickname = std::nullopt, std::optional<std::string> packetKey = std::nullopt);
        static std::vector<unsigned char> getReconnectionResultPacket(
            bool reconnectedSuccessfully,
            const std::string& uid,
            const std::string& receiverNicknameHash,
            const std::string& receiverToken,
            std::optional<bool> activeCall = std::nullopt,
            const std::string& callPartnerNicknameHash = "",
            std::optional<bool> isInMeeting = std::nullopt,
            std::optional<std::string> meetingRosterJson = std::nullopt);
        static std::vector<unsigned char> getUserInfoResultPacket(bool userInfoFound, const std::string& uid, const std::string& userNicknameHash, std::optional<CryptoPP::RSA::PublicKey> userPublicKey = std::nullopt, std::optional<std::string> encryptedNickname = std::nullopt, std::optional<std::string> packetKey = std::nullopt);
        static std::pair<std::string, std::vector<unsigned char>> getConnectionDownWithUserPacket(const std::string& userNicknameHash);
        static std::pair<std::string, std::vector<unsigned char>> getConnectionRestoredWithUserPacket(const std::string& userNicknameHash);
        static std::pair<std::string, std::vector<unsigned char>> getUserLogoutPacket(const std::string& userNicknameHash);
        static std::vector<unsigned char> getCallEndPacket(const std::string& senderNicknameHash);
        static std::vector<unsigned char> getMeetingCreateResultPacket(
            bool success,
            const std::string& meetingId = "",
            std::optional<std::string> encryptedMeetingKey = std::nullopt,
            std::optional<std::string> packetKey = std::nullopt);
        static std::vector<unsigned char> getMeetingInfoResultPacket(bool found, const std::string& ownerPublicKey = "");
        static std::vector<unsigned char> getMeetingEndedPacket();
        static std::vector<unsigned char> getMeetingParticipantJoinedPacket(const std::string& encryptedNickname, const std::string& serializedPublicKey);
        static std::vector<unsigned char> getMeetingParticipantLeftPacket(const std::string& nicknameHash);
        static std::vector<unsigned char> getMeetingJoinRejectedPacket(const std::string& reason);
        static std::vector<unsigned char> getMetricsResultPacket(
            double cpuUsagePercent,
            uint64_t memoryUsedBytes,
            uint64_t memoryAvailableBytes,
            const nlohmann::json& serverRuntime,
            const nlohmann::json& processes);

        // Helper packets for media sharing state (used for late joiners / reconnect).
        static std::vector<unsigned char> getMediaSharingBeginPacket(const std::string& senderNicknameHash);
        static std::vector<unsigned char> getMediaSharingEndPacket(const std::string& senderNicknameHash);
        static std::vector<unsigned char> getMuteBeginPacket(const std::string& senderNicknameHash);
        static std::vector<unsigned char> getMuteEndPacket(const std::string& senderNicknameHash);
    };
}