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
        std::function<void()> stopAudioSharing)
        : m_stateManager(stateManager)
        , m_eventListener(eventListener)
        , m_sendPacket(std::move(sendPacket))
        , m_startAudioSharing(std::move(startAudioSharing))
        , m_stopAudioSharing(std::move(stopAudioSharing))
    {
    }
    
    void ReconnectionPacketHandler::handleReconnectResult(const nlohmann::json& jsonObject) {
        m_stateManager->setConnectionDown(false);

        bool reconnected = jsonObject[RESULT].get<bool>();

        if (reconnected) {
            LOG_INFO("Connection restored successfully");

            m_eventListener->onConnectionRestored();

            bool activeCall = jsonObject[IS_ACTIVE_CALL].get<bool>();

            if (activeCall) {
                bool clientInCall = m_stateManager->isActiveCall();
                if (!clientInCall && jsonObject.contains(CALL_PARTNER_NICKNAME_HASH) && m_sendPacket) {
                    const std::string& partnerNicknameHash = jsonObject[CALL_PARTNER_NICKNAME_HASH].get<std::string>();
                    std::string myNicknameHash = utilities::crypto::calculateHash(m_stateManager->getMyNickname());
                    auto packet = PacketFactory::getCallEndPacketWithHashes(myNicknameHash, partnerNicknameHash);
                    m_sendPacket(packet, PacketType::CALL_END);
                    m_stateManager->resetOutgoingCall();
                    m_stateManager->resetActiveCall();
                    m_stateManager->setCallParticipantConnectionDown(false);
                    m_eventListener->onCallEndedByRemote({});
                    LOG_INFO("Reconnect: server said in call but client was not, ended call");
                }
                else if (clientInCall && jsonObject.contains(CALL_PARTNER_NICKNAME_HASH) && m_sendPacket) {
                    const std::string& serverPartnerHash = jsonObject[CALL_PARTNER_NICKNAME_HASH].get<std::string>();
                    std::string clientPartnerHash = utilities::crypto::calculateHash(m_stateManager->getActiveCall().getNickname());
                    if (serverPartnerHash != clientPartnerHash) {
                        std::string myNicknameHash = utilities::crypto::calculateHash(m_stateManager->getMyNickname());
                        auto packet = PacketFactory::getCallEndPacketWithHashes(myNicknameHash, serverPartnerHash);
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
        }
        else {
            LOG_INFO("Connection restored but authorization needed again");
            m_eventListener->onConnectionRestoredAuthorizationNeeded();
        }
    }
}