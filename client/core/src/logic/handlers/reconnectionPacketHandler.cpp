#include "reconnectionPacketHandler.h"
#include "logic/clientStateManager.h"
#include "logic/packetFactory.h"
#include "utilities/logger.h"
#include "utilities/crypto.h"
#include "constants/jsonType.h"
#include "constants/packetType.h"

using namespace core;
using namespace core::constant;

namespace core::logic
{
    ReconnectionPacketHandler::ReconnectionPacketHandler(
        std::shared_ptr<ClientStateManager> stateManager,
        std::shared_ptr<EventListener> eventListener,
        SendPacket sendPacket,
        std::function<void()> startAudioSharing,
        std::function<void()> stopAudioSharing,
        std::function<void()> stopScreenSharing,
        std::function<void()> stopCameraSharing)
        : m_stateManager(stateManager)
        , m_eventListener(eventListener)
        , m_sendPacket(std::move(sendPacket))
        , m_startAudioSharing(std::move(startAudioSharing))
        , m_stopAudioSharing(std::move(stopAudioSharing))
        , m_stopScreenSharing(std::move(stopScreenSharing))
        , m_stopCameraSharing(std::move(stopCameraSharing))
    {
    }
    
    void ReconnectionPacketHandler::handleReconnectResult(const nlohmann::json& jsonObject) {
        m_stateManager->setConnectionDown(false);

        bool reconnected = jsonObject[RESULT].get<bool>();

        if (reconnected) {
            LOG_INFO("Connection restored successfully");

            m_eventListener->onConnectionEstablished();

            bool activeCall = jsonObject[IS_ACTIVE_CALL].get<bool>();

            if (activeCall) {
                bool clientInCall = m_stateManager->isActiveCall();
                if (!clientInCall && jsonObject.contains(CALL_PARTNER_NICKNAME_HASH) && m_sendPacket) {
                    const std::string& partnerNicknameHash = jsonObject[CALL_PARTNER_NICKNAME_HASH].get<std::string>();
                    std::string myNicknameHash = utilities::crypto::calculateHash(m_stateManager->getMyNickname());
                    auto packet = PacketFactory::getCallEndPacketWithHashes(myNicknameHash);
                    m_sendPacket(packet, PacketType::CALL_END);
                    m_stateManager->resetOutgoingCall();
                    m_stateManager->resetActiveCall();
                    m_stateManager->setCallParticipantConnectionDown(false);
                    m_eventListener->onCallEndedByRemote({});
                    LOG_INFO("Reconnect: server said in call but client was not, ended call");
                }
                else if (clientInCall && jsonObject.contains(CALL_PARTNER_NICKNAME_HASH) && m_sendPacket) {
                    const std::string& serverPartnerHash = jsonObject[CALL_PARTNER_NICKNAME_HASH].get<std::string>();
                    auto activeOpt = m_stateManager->getActiveCall();
                    if (activeOpt) {
                    std::string clientPartnerHash = utilities::crypto::calculateHash(activeOpt->get().getNickname());
                    if (serverPartnerHash != clientPartnerHash) {
                        std::string myNicknameHash = utilities::crypto::calculateHash(m_stateManager->getMyNickname());
                        auto packet = PacketFactory::getCallEndPacketWithHashes(myNicknameHash);
                        m_sendPacket(packet, PacketType::CALL_END);
                        m_stateManager->resetActiveCall();
                        m_stateManager->setCallParticipantConnectionDown(false);
                        if (m_stopAudioSharing) m_stopAudioSharing();
                        m_eventListener->onCallEndedByRemote({});
                        LOG_INFO("Reconnect: server said in call with different partner, ended call");
                    }
                    else if (m_startAudioSharing) {
                        m_startAudioSharing();
                    }
                    }
                }
                else if (clientInCall && m_startAudioSharing) {
                    m_startAudioSharing();
                }
            }

            if (!activeCall) {
                if (m_stateManager->isOutgoingCall()) {
                    m_stateManager->resetOutgoingCall();
                    m_eventListener->onOutgoingCallTimeout({});
                }

                bool hadActiveCall = m_stateManager->isActiveCall();
                m_stateManager->resetActiveCall();
                m_stateManager->setCallParticipantConnectionDown(false);
                if (hadActiveCall) {
                    m_eventListener->onCallEndedByRemote({});
                }
            }

            // Handle meeting state mismatch on reconnect.
            if (jsonObject.contains(IS_IN_MEETING)) {
                bool serverInMeeting = jsonObject[IS_IN_MEETING].get<bool>();
                bool clientInMeeting = m_stateManager->isActiveMeeting();

                if (!serverInMeeting && clientInMeeting) {
                    // Server считает, что мы уже не в митинге — локально всё сбрасываем.
                    if (m_stopScreenSharing) m_stopScreenSharing();
                    if (m_stopCameraSharing) m_stopCameraSharing();
                    if (m_stopAudioSharing) m_stopAudioSharing();
                    m_stateManager->resetActiveMeeting();
                    m_stateManager->resetIncomingMeetingJoinRequests();
                    m_eventListener->onMeetingEndedByOwner();
                } else if (serverInMeeting && !clientInMeeting && m_sendPacket) {
                    // Клиент уже вышел локально, а сервер всё ещё держит в митинге:
                    // пробуем синхронизировать серверное состояние существующими пакетами.
                    const std::string myNickname = m_stateManager->getMyNickname();
                    auto endPacket = PacketFactory::getMeetingEndPacket(myNickname);
                    auto endEc = m_sendPacket(endPacket, PacketType::MEETING_END);
                    if (endEc) {
                        LOG_WARN("Reconnect: failed to send MEETING_END on state mismatch: {}", endEc.message());
                    }

                    auto leavePacket = PacketFactory::getMeetingLeavePacket(myNickname);
                    auto leaveEc = m_sendPacket(leavePacket, PacketType::MEETING_LEAVE);
                    if (leaveEc) {
                        LOG_WARN("Reconnect: failed to send MEETING_LEAVE on state mismatch: {}", leaveEc.message());
                    } else {
                        LOG_INFO("Reconnect: server in meeting but client not, sent MEETING_END and MEETING_LEAVE");
                    }
                } else if (serverInMeeting && clientInMeeting) {
                    // При успешном reconnect в активный митинг сбрасываем флаги просмотра,
                    // фактическое состояние придёт отдельными *_SHARING_BEGIN пакетами.
                    m_stateManager->setViewingRemoteScreen(false);
                    m_stateManager->setViewingRemoteCamera(false);
                }
            }
        }
        else {
            LOG_INFO("Connection restored but authorization needed again");
            m_stateManager->reset();
            m_eventListener->onConnectionEstablishedAuthorizationNeeded();
        }
    }
}