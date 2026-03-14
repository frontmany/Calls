#include "packetFactory.h"
#include "constants/jsonType.h"

#include "json.hpp"

using namespace core::constant;
using namespace core::utilities::crypto;

namespace core::logic
{
    namespace
    {
        std::vector<unsigned char> toBytes(const std::string& value) {
            return std::vector<unsigned char>(value.begin(), value.end());
        }

        nlohmann::json createBasePacket(const std::string& uid, const std::string& senderNickname) {
            nlohmann::json jsonObject;
            jsonObject[UID] = uid;
            jsonObject[SENDER_NICKNAME_HASH] = calculateHash(senderNickname);
            return jsonObject;
        }

        nlohmann::json createSenderReceiverPacket(const std::string& uid, const std::string& senderNickname, const std::string& receiverNickname) {
            nlohmann::json jsonObject = createBasePacket(uid, senderNickname);
            jsonObject[RECEIVER_NICKNAME_HASH] = calculateHash(receiverNickname);
            return jsonObject;
        }

        std::vector<unsigned char> createSenderReceiverPacketBytes(const std::string& myNickname, const std::string& peerNickname) {
            std::string uid = generateUID();
            nlohmann::json jsonObject = createSenderReceiverPacket(uid, myNickname, peerNickname);
            return toBytes(jsonObject.dump());
        }
    }

    std::vector<unsigned char> PacketFactory::getAuthorizationRequestPacket(const std::string& myNickname, const CryptoPP::RSA::PublicKey& myPublicKey, uint16_t myUdpPort) {
        CryptoPP::SecByteBlock packetKey;
        generateAESKey(packetKey);
        std::string uid = generateUID();

        nlohmann::json jsonObject = createBasePacket(uid, myNickname);
        jsonObject[ENCRYPTED_NICKNAME] = AESEncrypt(packetKey, myNickname);
        jsonObject[PUBLIC_KEY] = serializePublicKey(myPublicKey);
        jsonObject[UDP_PORT] = myUdpPort;
        jsonObject[PACKET_KEY] = RSAEncryptAESKey(myPublicKey, packetKey);

        return toBytes(jsonObject.dump());
    }

    std::vector<unsigned char> PacketFactory::getLogoutPacket(const std::string& myNickname) {
        std::string uid = generateUID();
        nlohmann::json jsonObject = createBasePacket(uid, myNickname);
        return toBytes(jsonObject.dump());
    }

    std::vector<unsigned char> PacketFactory::getReconnectionRequestPacket(const std::string& myNickname, const std::string& myToken, uint16_t myUdpPort) {
        std::string uid = generateUID();
        nlohmann::json jsonObject = createBasePacket(uid, myNickname);
        jsonObject[TOKEN] = myToken;
        jsonObject[UDP_PORT] = myUdpPort;
        return toBytes(jsonObject.dump());
    }

    std::vector<unsigned char> PacketFactory::getMetricsRequestPacket(const std::string& myNickname) {
        std::string uid = generateUID();
        nlohmann::json jsonObject = createBasePacket(uid, myNickname);
        return toBytes(jsonObject.dump());
    }

    std::vector<unsigned char> PacketFactory::getUserInfoRequestPacket(const std::string& myNickname, const CryptoPP::RSA::PublicKey& myPublicKey, const std::string& userNickname) {
        CryptoPP::SecByteBlock packetKey;
        generateAESKey(packetKey);
        std::string uid = generateUID();

        nlohmann::json jsonObject = createBasePacket(uid, myNickname);
        jsonObject[ENCRYPTED_NICKNAME] = AESEncrypt(packetKey, userNickname);
        jsonObject[NICKNAME_HASH] = calculateHash(userNickname);
        jsonObject[PACKET_KEY] = RSAEncryptAESKey(myPublicKey, packetKey);
        return toBytes(jsonObject.dump());
    }

    std::vector<unsigned char> PacketFactory::getOutgoingCallBeginPacket(
        const std::string& myNickname,
        const std::string& userNickname,
        const CryptoPP::RSA::PublicKey& myPublicKey,
        const CryptoPP::RSA::PublicKey& userPublicKey,
        const CryptoPP::SecByteBlock& callKey)
    {
        CryptoPP::SecByteBlock packetKey;
        generateAESKey(packetKey);
        std::string uid = generateUID();

        nlohmann::json jsonObject = createSenderReceiverPacket(uid, myNickname, userNickname);
        jsonObject[ENCRYPTED_CALL_KEY] = RSAEncryptAESKey(userPublicKey, callKey);
        jsonObject[SENDER_PUBLIC_KEY] = serializePublicKey(myPublicKey);
        jsonObject[SENDER_ENCRYPTED_NICKNAME] = AESEncrypt(packetKey, myNickname);
        jsonObject[PACKET_KEY] = RSAEncryptAESKey(userPublicKey, packetKey);

        return toBytes(jsonObject.dump());
    }

    std::vector<unsigned char> PacketFactory::getCallAcceptPacket(
        const std::string& myNickname,
        const std::string& userNickname,
        const CryptoPP::RSA::PublicKey& myPublicKey,
        const CryptoPP::RSA::PublicKey& userPublicKey,
        const CryptoPP::SecByteBlock& callKey)
    {
        return getOutgoingCallBeginPacket(myNickname, userNickname, myPublicKey, userPublicKey, callKey);
    }

    namespace
    {
        std::vector<unsigned char> createBasePacketBytes(const std::string& myNickname) {
            std::string uid = generateUID();
            nlohmann::json jsonObject = createBasePacket(uid, myNickname);
            return toBytes(jsonObject.dump());
        }
    }

    std::vector<unsigned char> PacketFactory::getCallingEndPacket(const std::string& myNickname) {
        return createBasePacketBytes(myNickname);
    }

    std::vector<unsigned char> PacketFactory::getCallDeclinePacket(const std::string& myNickname, const std::string& peerNickname) {
        return createSenderReceiverPacketBytes(myNickname, peerNickname);
    }

    std::vector<unsigned char> PacketFactory::getCallEndPacket(const std::string& myNickname) {
        return createBasePacketBytes(myNickname);
    }

    std::vector<unsigned char> PacketFactory::getCallEndPacketWithHashes(const std::string& senderNicknameHash) {
        std::string uid = generateUID();
        nlohmann::json jsonObject;
        jsonObject[UID] = uid;
        jsonObject[SENDER_NICKNAME_HASH] = senderNicknameHash;
        return toBytes(jsonObject.dump());
    }

    std::vector<unsigned char> PacketFactory::getScreenSharingBeginPacket(const std::string& myNickname) {
        return createBasePacketBytes(myNickname);
    }

    std::vector<unsigned char> PacketFactory::getScreenSharingEndPacket(const std::string& myNickname) {
        return createBasePacketBytes(myNickname);
    }

    std::vector<unsigned char> PacketFactory::getCameraSharingBeginPacket(const std::string& myNickname) {
        return createBasePacketBytes(myNickname);
    }

    std::vector<unsigned char> PacketFactory::getCameraSharingEndPacket(const std::string& myNickname) {
        return createBasePacketBytes(myNickname);
    }

    std::vector<unsigned char> PacketFactory::getMeetingCreatePacket(const std::string& myNickname) {
        return createBasePacketBytes(myNickname);
    }

    std::vector<unsigned char> PacketFactory::getGetMeetingInfoRequestPacket(const std::string& myNickname, const std::string& meetingId) {
        std::string uid = generateUID();
        nlohmann::json jsonObject = createBasePacket(uid, myNickname);
        jsonObject[MEETING_ID_HASH] = calculateHash(meetingId);
        return toBytes(jsonObject.dump());
    }

    std::vector<unsigned char> PacketFactory::getMeetingJoinRequestPacket(
        const std::string& myNickname,
        const std::string& meetingId,
        const CryptoPP::RSA::PublicKey& myPublicKey,
        const CryptoPP::RSA::PublicKey& ownerPublicKey)
    {
        CryptoPP::SecByteBlock packetKey;
        generateAESKey(packetKey);
        std::string uid = generateUID();

        nlohmann::json jsonObject = createBasePacket(uid, myNickname);
        jsonObject[MEETING_ID_HASH] = calculateHash(meetingId);
        jsonObject[SENDER_PUBLIC_KEY] = serializePublicKey(myPublicKey);
        jsonObject[ENCRYPTED_NICKNAME] = AESEncrypt(packetKey, myNickname);
        jsonObject[PACKET_KEY] = RSAEncryptAESKey(ownerPublicKey, packetKey);

        return toBytes(jsonObject.dump());
    }

    std::vector<unsigned char> PacketFactory::getMeetingJoinCancelPacket(const std::string& myNickname) {
        return createBasePacketBytes(myNickname);
    }

    std::vector<unsigned char> PacketFactory::getMeetingJoinAcceptPacket(
        const std::string& myNickname,
        const std::string& friendNickname,
        const CryptoPP::RSA::PublicKey& requesterPublicKey,
        const CryptoPP::SecByteBlock& meetingKey,
        const std::vector<core::MeetingParticipant>& participants)
    {
        std::string uid = generateUID();
        nlohmann::json jsonObject = createBasePacket(uid, myNickname);
        jsonObject[REQUESTER_NICKNAME_HASH] = calculateHash(friendNickname);
        jsonObject[ENCRYPTED_NICKNAME] = AESEncrypt(meetingKey, friendNickname);
        jsonObject[ENCRYPTED_MEETING_KEY] = RSAEncryptAESKey(requesterPublicKey, meetingKey);
        nlohmann::json participantsJson = nlohmann::json::array();
        for (const auto& p : participants) {
            const auto& user = p.getUser();
            nlohmann::json obj;
            obj[NICKNAME] = user.getNickname();
            obj[PUBLIC_KEY] = serializePublicKey(static_cast<const CryptoPP::RSA::PublicKey&>(user.getPublicKey()));
            obj[IS_OWNER] = p.isOwner();
            participantsJson.push_back(std::move(obj));
        }
        jsonObject[ENCRYPTED_PARTICIPANTS] = AESEncrypt(meetingKey, participantsJson.dump());
        return toBytes(jsonObject.dump());
    }

    std::vector<unsigned char> PacketFactory::getMeetingJoinDeclinePacket(const std::string& myNickname, const std::string& friendNickname) {
        std::string uid = generateUID();
        nlohmann::json jsonObject = createBasePacket(uid, myNickname);
        jsonObject[REQUESTER_NICKNAME_HASH] = calculateHash(friendNickname);
        return toBytes(jsonObject.dump());
    }

    std::vector<unsigned char> PacketFactory::getMeetingLeavePacket(const std::string& myNickname) {
        return createBasePacketBytes(myNickname);
    }

    std::vector<unsigned char> PacketFactory::getMeetingEndPacket(const std::string& myNickname) {
        return createBasePacketBytes(myNickname);
    }
}
