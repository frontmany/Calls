#pragma once 

#include <string>
#include <vector>
#include "models/meetingParticipant.h"
#include "utilities/crypto.h"

namespace core::logic 
{
    class PacketFactory {
    public:
        PacketFactory() = default;

        // Authorization & session
        static std::vector<unsigned char> getAuthorizationRequestPacket(const std::string& myNickname, const CryptoPP::RSA::PublicKey& myPublicKey, uint16_t myUdpPort);
        static std::vector<unsigned char> getLogoutPacket(const std::string& myNickname);
        static std::vector<unsigned char> getReconnectionRequestPacket(const std::string& myNickname, const std::string& myToken, uint16_t myUdpPort);
        static std::vector<unsigned char> getMetricsRequestPacket(const std::string& myNickname);

        // User info (for starting call)
        static std::vector<unsigned char> getUserInfoRequestPacket(const std::string& myNickname, const CryptoPP::RSA::PublicKey& myPublicKey, const std::string& userNickname);

        // Call: start / accept / end / decline
        static std::vector<unsigned char> getOutgoingCallBeginPacket(
            const std::string& myNickname,
            const std::string& userNickname,
            const CryptoPP::RSA::PublicKey& myPublicKey,
            const CryptoPP::RSA::PublicKey& userPublicKey,
            const CryptoPP::SecByteBlock& callKey);

        static std::vector<unsigned char> getCallAcceptPacket(
            const std::string& myNickname,
            const std::string& userNickname,
            const CryptoPP::RSA::PublicKey& myPublicKey,
            const CryptoPP::RSA::PublicKey& userPublicKey,
            const CryptoPP::SecByteBlock& callKey);

        static std::vector<unsigned char> getCallingEndPacket(const std::string& myNickname);
        static std::vector<unsigned char> getCallDeclinePacket(const std::string& myNickname, const std::string& peerNickname);
        static std::vector<unsigned char> getCallEndPacket(const std::string& myNickname);
        static std::vector<unsigned char> getCallEndPacketWithHashes(const std::string& senderNicknameHash);

        // Media (screen / camera)
        static std::vector<unsigned char> getScreenSharingBeginPacket(const std::string& myNickname);
        static std::vector<unsigned char> getScreenSharingEndPacket(const std::string& myNickname);
        static std::vector<unsigned char> getCameraSharingBeginPacket(const std::string& myNickname);
        static std::vector<unsigned char> getCameraSharingEndPacket(const std::string& myNickname);

        // Meeting
        static std::vector<unsigned char> getMeetingCreatePacket(const std::string& myNickname);
        static std::vector<unsigned char> getGetMeetingInfoRequestPacket(const std::string& myNickname, const std::string& meetingId);
        static std::vector<unsigned char> getMeetingJoinRequestPacket(
            const std::string& myNickname,
            const std::string& meetingId,
            const CryptoPP::RSA::PublicKey& myPublicKey,
            const CryptoPP::RSA::PublicKey& ownerPublicKey);
        static std::vector<unsigned char> getMeetingJoinCancelPacket(const std::string& myNickname);
        static std::vector<unsigned char> getMeetingJoinAcceptPacket(
            const std::string& myNickname,
            const std::string& friendNickname,
            const CryptoPP::RSA::PublicKey& requesterPublicKey,
            const CryptoPP::SecByteBlock& meetingKey,
            const std::vector<core::MeetingParticipant>& participants);
        static std::vector<unsigned char> getMeetingJoinDeclinePacket(const std::string& myNickname, const std::string& friendNickname);
        static std::vector<unsigned char> getMeetingLeavePacket(const std::string& myNickname);
        static std::vector<unsigned char> getMeetingEndPacket(const std::string& myNickname);
    };
}