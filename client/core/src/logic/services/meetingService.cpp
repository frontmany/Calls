#include "meetingService.h"
#include "constants/errorCode.h"
#include "constants/packetType.h"
#include "logic/packetFactory.h"
#include "utilities/crypto.h"

using namespace core::constant;

namespace core::logic
{
    MeetingService::MeetingService(
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

    std::error_code MeetingService::createMeeting() {
        if (m_stateManager->isConnectionDown()) return make_error_code(ErrorCode::connection_down);
        if (!m_stateManager->isAuthorized()) return make_error_code(ErrorCode::not_authorized);
        if (m_stateManager->isActiveCall()) return make_error_code(ErrorCode::active_call_exists);
        if (m_stateManager->isOutgoingCall()) return make_error_code(ErrorCode::operation_in_progress);
        if (m_stateManager->isActiveMeeting()) return make_error_code(ErrorCode::in_meeting);
        if (!m_keyManager) return make_error_code(ErrorCode::network_error);

        CryptoPP::SecByteBlock meetingKey;
        core::utilities::crypto::generateAESKey(meetingKey);

        auto packet = PacketFactory::getMeetingCreatePacket(
            m_stateManager->getMyNickname(),
            m_keyManager->getMyPublicKey(),
            meetingKey);
        auto ec = m_sendPacket(packet, PacketType::MEETING_CREATE);
        if (ec) return ec;

        return {};
    }

    std::error_code MeetingService::joinMeeting(const std::string& meetingId) {
        if (m_stateManager->isConnectionDown()) return make_error_code(ErrorCode::connection_down);
        if (!m_stateManager->isAuthorized()) return make_error_code(ErrorCode::not_authorized);
        if (m_stateManager->isActiveCall()) return make_error_code(ErrorCode::active_call_exists);
        if (m_stateManager->isOutgoingCall()) return make_error_code(ErrorCode::operation_in_progress);
        if (m_stateManager->isActiveMeeting()) return make_error_code(ErrorCode::in_meeting);
        if (m_stateManager->isOutgoingJoinMeetingRequest()) return make_error_code(ErrorCode::operation_in_progress);

        auto packet = PacketFactory::getGetMeetingInfoRequestPacket(m_stateManager->getMyNickname(), meetingId);
        auto ec = m_sendPacket(packet, PacketType::GET_MEETING_INFO);
        if (ec) return ec;

        auto stateManager = m_stateManager;
        auto eventListener = m_eventListener;
        m_stateManager->setOutgoingJoinMeetingRequest(meetingId, kJoinMeetingRequestTimeout,
            [stateManager, eventListener]() {
                stateManager->resetOutgoingJoinMeetingRequest();
                if (eventListener) eventListener->onJoinMeetingRequestTimeout();
            });
        return {};
    } 

    std::error_code MeetingService::cancelMeetingJoin() {
        if (m_stateManager->isConnectionDown()) return make_error_code(ErrorCode::connection_down);
        if (!m_stateManager->isAuthorized()) return make_error_code(ErrorCode::not_authorized);
        if (!m_stateManager->isOutgoingJoinMeetingRequest()) return make_error_code(ErrorCode::no_pending_join_request);

        auto reqOpt = m_stateManager->getOutgoingJoinMeetingRequest();
        if (!reqOpt) return make_error_code(ErrorCode::no_pending_join_request);
        auto packet = PacketFactory::getMeetingJoinCancelPacket(m_stateManager->getMyNickname());
        auto ec = m_sendPacket(packet, PacketType::MEETING_JOIN_CANCEL);
        if (ec) return ec;

        m_stateManager->resetOutgoingJoinMeetingRequest();
        return {};
    }

    std::error_code MeetingService::acceptJoinMeetingRequest(const std::string& friendNickname) {
        if (m_stateManager->isConnectionDown()) return make_error_code(ErrorCode::connection_down);
        if (!m_stateManager->isAuthorized()) return make_error_code(ErrorCode::not_authorized);


        auto meetingOpt = m_stateManager->getActiveMeeting();
        if (!meetingOpt) return make_error_code(ErrorCode::not_in_meeting);

        auto owner = meetingOpt->get().getOwner();
        if (!owner.has_value() || owner->getUser().getNickname() != m_stateManager->getMyNickname()) return make_error_code(ErrorCode::not_meeting_owner);
        
        const auto& requests = m_stateManager->getIncomingMeetingJoinRequests();
        auto requestIt = requests.find(friendNickname);
        if (requestIt == requests.end()) return make_error_code(ErrorCode::no_meeting_join_request);

        auto meetingKey = meetingOpt->get().getMeetingKey();
        auto packet = PacketFactory::getMeetingJoinAcceptPacket(
            m_stateManager->getMyNickname(),
            friendNickname,
            requestIt->second.getPublicKey(),
            meetingKey,
            meetingOpt->get().getParticipants());
        auto ec = m_sendPacket(packet, PacketType::MEETING_JOIN_ACCEPT);
        if (ec) return ec;

        m_stateManager->removeIncomingMeetingJoinRequest(friendNickname);
        return {};
    }

    std::error_code MeetingService::declineJoinMeetingRequest(const std::string& friendNickname) {
        if (m_stateManager->isConnectionDown()) return make_error_code(ErrorCode::connection_down);
        if (!m_stateManager->isAuthorized()) return make_error_code(ErrorCode::not_authorized);
        auto meetingOptDecline = m_stateManager->getActiveMeeting();
        if (!meetingOptDecline) return make_error_code(ErrorCode::not_in_meeting);
        auto ownerDecline = meetingOptDecline->get().getOwner();
        if (!ownerDecline.has_value() || ownerDecline->getUser().getNickname() != m_stateManager->getMyNickname()) return make_error_code(ErrorCode::not_meeting_owner);
        if (!m_stateManager->getIncomingMeetingJoinRequests().contains(friendNickname)) return make_error_code(ErrorCode::no_meeting_join_request);

        auto packet = PacketFactory::getMeetingJoinDeclinePacket(m_stateManager->getMyNickname(), friendNickname);
        auto ec = m_sendPacket(packet, PacketType::MEETING_JOIN_DECLINE);
        if (ec) return ec;

        m_stateManager->removeIncomingMeetingJoinRequest(friendNickname);
        return {};
    } 

    std::error_code MeetingService::endMeeting() {
        if (m_stateManager->isConnectionDown()) return make_error_code(ErrorCode::connection_down);
        if (!m_stateManager->isAuthorized()) return make_error_code(ErrorCode::not_authorized);
        auto meetingOptEnd = m_stateManager->getActiveMeeting();
        if (!meetingOptEnd) return make_error_code(ErrorCode::not_in_meeting);
        auto ownerEnd = meetingOptEnd->get().getOwner();
        if (!ownerEnd.has_value() || ownerEnd->getUser().getNickname() != m_stateManager->getMyNickname()) return make_error_code(ErrorCode::not_meeting_owner);

        auto packet = PacketFactory::getMeetingEndPacket(m_stateManager->getMyNickname());
        auto ec = m_sendPacket(packet, PacketType::MEETING_END);
        if (ec) return ec;

        m_stateManager->resetActiveMeeting();
        m_stateManager->resetIncomingMeetingJoinRequests();
        return {};
    }

    std::error_code MeetingService::leaveMeeting() {
        if (m_stateManager->isConnectionDown()) return make_error_code(ErrorCode::connection_down);
        if (!m_stateManager->isAuthorized()) return make_error_code(ErrorCode::not_authorized);
        if (!m_stateManager->isActiveMeeting()) return make_error_code(ErrorCode::not_in_meeting);

        auto packet = PacketFactory::getMeetingLeavePacket(m_stateManager->getMyNickname());
        auto ec = m_sendPacket(packet, PacketType::MEETING_LEAVE);
        if (ec) return ec;

        m_stateManager->resetActiveMeeting();
        m_stateManager->resetIncomingMeetingJoinRequests();
        return {};
    }

}
