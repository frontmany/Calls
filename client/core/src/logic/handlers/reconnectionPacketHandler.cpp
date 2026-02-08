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
        std::shared_ptr<EventListener> eventListener)
        : m_stateManager(stateManager)
        , m_eventListener(eventListener)
    {
    }
    
    void ReconnectionPacketHandler::handleReconnectResult(const nlohmann::json& jsonObject) {
        m_stateManager->setConnectionDown(false);

        bool reconnected = jsonObject[RESULT].get<bool>();

        if (reconnected) {
            LOG_INFO("Connection restored successfully");

            m_eventListener->onConnectionRestored();

            bool activeCall = jsonObject[IS_ACTIVE_CALL].get<bool>();

            if (!activeCall) {
                m_stateManager->resetActiveCall();

                bool hadActiveCall = m_stateManager->isActiveCall();
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