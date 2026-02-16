#include "reconnectionPacketHandler.h"
#include "logic/clientStateManager.h"
#include "logic/keyManager.h"
#include "utilities/logger.h"
#include "constants/jsonType.h"


using namespace core;
using namespace core::constant;
using namespace core::utilities::crypto;

namespace core::logic
{
    ReconnectionPacketHandler::ReconnectionPacketHandler(
        std::shared_ptr<ClientStateManager> stateManager,
        std::shared_ptr<EventListener> eventListener,
        std::function<void()> onRestoredToActiveCall)
        : m_stateManager(stateManager)
        , m_eventListener(eventListener)
        , m_onRestoredToActiveCall(std::move(onRestoredToActiveCall))
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
                if (!m_stateManager->isActiveCall() && m_stateManager->isOutgoingCall()) {
                    const auto& outgoingCall = m_stateManager->getOutgoingCall();
                    if (outgoingCall.hasCallContext()) {
                        std::string nickname = outgoingCall.getNickname();
                        m_stateManager->setActiveCall(
                            nickname,
                            outgoingCall.getPublicKey(),
                            outgoingCall.getCallKey());
                        m_eventListener->onOutgoingCallAcceptedWithNickname(nickname);
                        if (m_onRestoredToActiveCall) {
                            m_onRestoredToActiveCall();
                        }
                    }
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