#include "meetingPacketHandler.h"
#include "logic/clientStateManager.h"
#include "logic/keyManager.h"
#include "logic/packetFactory.h"
#include "constants/errorCode.h"
#include "constants/jsonType.h"
#include "utilities/crypto.h"

using namespace core::constant;
using namespace core::utilities::crypto;

namespace core::logic
{
    namespace
    {
        CryptoPP::SecByteBlock deriveMeetingKey(const std::string& meetingId) {
            auto binOpt = hashToBinary(calculateHash(meetingId));
            if (!binOpt) return {};
            return CryptoPP::SecByteBlock(binOpt->data(), binOpt->size());
        }
    }

    MeetingPacketHandler::MeetingPacketHandler(
        std::shared_ptr<ClientStateManager> stateManager,
        std::shared_ptr<KeyManager> keyManager,
        std::shared_ptr<EventListener> eventListener,
        std::function<std::error_code(const std::vector<unsigned char>&, core::constant::PacketType)>&& sendPacket)
        : m_stateManager(std::move(stateManager))
        , m_keyManager(std::move(keyManager))
        , m_eventListener(std::move(eventListener))
        , m_sendPacket(std::move(sendPacket))
    {
    }

    void MeetingPacketHandler::handleGetMeetingInfoResult(const nlohmann::json& jsonObject) {
        if (!m_stateManager->isAuthorized() || m_stateManager->isConnectionDown()) return;
        if (!m_stateManager->isOutgoingJoinMeetingRequest()) return;

        bool found = jsonObject.contains(RESULT) && jsonObject[RESULT].get<bool>();
        if (!found) {
            m_stateManager->resetOutgoingJoinMeetingRequest();
            if (m_eventListener) m_eventListener->onJoinMeetingDeclined(""); 
            return;
        }

        auto reqOpt = m_stateManager->getOutgoingJoinMeetingRequest();
        if (!reqOpt) return;
        std::string meetingId = reqOpt->get().getMeetingId();

        const std::string& serializedOwnerPublicKey = jsonObject[PUBLIC_KEY].get<std::string>();
        auto ownerPublicKey = deserializePublicKey(serializedOwnerPublicKey);

        auto packet = PacketFactory::getMeetingJoinRequestPacket(
            m_stateManager->getMyNickname(), meetingId,
            m_keyManager->getMyPublicKey(), ownerPublicKey);

        m_sendPacket(packet, PacketType::MEETING_JOIN_REQUEST);
    }

    void MeetingPacketHandler::handleMeetingCreateResult(const nlohmann::json& jsonObject) {
        if (!m_stateManager->isAuthorized() || m_stateManager->isConnectionDown()) return;

        bool success = false;
        if (jsonObject.contains(RESULT)) {
            const auto& r = jsonObject[RESULT];
            if (r.is_boolean()) success = r.get<bool>();
            else if (r.is_string()) success = (r.get<std::string>() == "ok");
        }

        if (success) {
            if (m_stateManager->isActiveMeeting()) return;
            std::string meetingId;
            if (jsonObject.contains(MEETING_ID)) meetingId = jsonObject[MEETING_ID].get<std::string>();
            if (meetingId.empty()) return;
            std::string meetingIdForEvent = meetingId;
            m_stateManager->setActiveMeeting(std::move(meetingId));
            if (m_eventListener) {
                m_eventListener->onMeetingCreated(meetingIdForEvent);
            }
        } else {
            if (m_eventListener) {
                m_eventListener->onMeetingCreateRejected(make_error_code(ErrorCode::meeting_create_rejected));
            }
        }
    }

    void MeetingPacketHandler::handleMeetingJoinRequestForward(const nlohmann::json& jsonObject) {
        if (!m_stateManager->isAuthorized() || m_stateManager->isConnectionDown()) return;
        auto meetingOpt = m_stateManager->getActiveMeeting();
        if (!meetingOpt) return;
        auto owner = meetingOpt->get().getOwner();
        if (!owner.has_value() || owner->getUser().getNickname() != m_stateManager->getMyNickname()) return;

        if (!jsonObject.contains(PACKET_KEY) || !jsonObject.contains(ENCRYPTED_NICKNAME)) return;

        const std::string& encryptedPacketKey = jsonObject[PACKET_KEY].get<std::string>();
        auto packetKey = RSADecryptAESKey(m_keyManager->getMyPrivateKey(), encryptedPacketKey);

        const std::string& encryptedNickname = jsonObject[ENCRYPTED_NICKNAME].get<std::string>();
        std::string nickname = AESDecrypt(packetKey, encryptedNickname);
        if (nickname.empty()) return;

        m_stateManager->addIncomingMeetingJoinRequest(nickname);
        if (m_eventListener) {
            m_eventListener->onMeetingJoinRequestReceived(nickname);
        }
    }

    void MeetingPacketHandler::handleMeetingJoinRequestCancelled(const nlohmann::json& jsonObject) {
        if (!m_stateManager->isAuthorized() || m_stateManager->isConnectionDown()) return;
        auto meetingOpt = m_stateManager->getActiveMeeting();
        if (!meetingOpt) return;
        auto owner = meetingOpt->get().getOwner();
        if (!owner.has_value() || owner->getUser().getNickname() != m_stateManager->getMyNickname()) return;

        if (!jsonObject.contains(SENDER_NICKNAME_HASH)) return;
        const std::string& senderHash = jsonObject[SENDER_NICKNAME_HASH].get<std::string>();

        const auto& requests = m_stateManager->getIncomingMeetingJoinRequests();
        std::string matchedNickname;
        for (const auto& [nickname, _] : requests) {
            if (calculateHash(nickname) == senderHash) {
                matchedNickname = nickname;
                break;
            }
        }
        if (matchedNickname.empty()) return;

        m_stateManager->removeIncomingMeetingJoinRequest(matchedNickname);
        if (m_eventListener) {
            m_eventListener->onMeetingJoinRequestCancelled(matchedNickname);
        }
    }

    void MeetingPacketHandler::handleMeetingJoinAccept(const nlohmann::json& jsonObject) {
        if (!m_stateManager->isAuthorized() || m_stateManager->isConnectionDown()) return;
        if (!m_stateManager->isOutgoingJoinMeetingRequest()) return;

        auto reqOpt = m_stateManager->getOutgoingJoinMeetingRequest();
        if (!reqOpt) return;
        std::string meetingId = reqOpt->get().getMeetingId();

        m_stateManager->resetOutgoingJoinMeetingRequest();
        m_stateManager->setActiveMeeting(meetingId);
        if (m_eventListener) {
            m_eventListener->onJoinMeetingAccepted(meetingId);
        }
    }

    void MeetingPacketHandler::handleMeetingJoinDecline(const nlohmann::json& jsonObject) {
        if (!m_stateManager->isAuthorized() || m_stateManager->isConnectionDown()) return;
        if (!m_stateManager->isOutgoingJoinMeetingRequest()) return;

        std::string meetingId;
        auto reqOpt = m_stateManager->getOutgoingJoinMeetingRequest();
        if (reqOpt) meetingId = reqOpt->get().getMeetingId();

        m_stateManager->resetOutgoingJoinMeetingRequest();
        if (m_eventListener) {
            m_eventListener->onJoinMeetingDeclined(meetingId);
        }
    }

    void MeetingPacketHandler::handleMeetingEnded(const nlohmann::json& jsonObject) {
        if (!m_stateManager->isAuthorized() || m_stateManager->isConnectionDown()) return;
        if (!m_stateManager->isActiveMeeting()) return;

        m_stateManager->resetActiveMeeting();
        m_stateManager->resetIncomingMeetingJoinRequests();
        if (m_eventListener) {
            m_eventListener->onMeetingEndedByOwner();
        }
    }

    void MeetingPacketHandler::handleMeetingParticipantJoined(const nlohmann::json& jsonObject) {
        if (!m_stateManager->isAuthorized() || m_stateManager->isConnectionDown()) return;
        if (!m_stateManager->isActiveMeeting()) return;

        auto meetingOpt = m_stateManager->getActiveMeeting();
        if (!meetingOpt || !jsonObject.contains(ENCRYPTED_NICKNAME)) return;

        auto meetingKey = deriveMeetingKey(meetingOpt->get().getMeetingId());
        std::string nickname = AESDecrypt(meetingKey, jsonObject[ENCRYPTED_NICKNAME].get<std::string>());
        if (nickname.empty()) return;

        if (m_eventListener) {
            m_eventListener->onMeetingParticipantJoined(nickname);
        }
    }

    void MeetingPacketHandler::handleMeetingParticipantLeft(const nlohmann::json& jsonObject) {
        if (!m_stateManager->isAuthorized() || m_stateManager->isConnectionDown()) return;
        if (!m_stateManager->isActiveMeeting()) return;

        auto meetingOpt = m_stateManager->getActiveMeeting();
        if (!meetingOpt || !jsonObject.contains(ENCRYPTED_NICKNAME)) return;

        auto meetingKey = deriveMeetingKey(meetingOpt->get().getMeetingId());
        std::string nickname = AESDecrypt(meetingKey, jsonObject[ENCRYPTED_NICKNAME].get<std::string>());
        if (nickname.empty()) return;

        if (m_eventListener) {
            m_eventListener->onMeetingParticipantLeft(nickname);
        }
    }
}
