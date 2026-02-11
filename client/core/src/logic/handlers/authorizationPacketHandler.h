#pragma once

#include <memory>
#include <string>
#include "eventListener.h"

#include "json.hpp"

namespace core::logic
{
    class ClientStateManager;
    class KeyManager;

    class AuthorizationPacketHandler {
    public:
        AuthorizationPacketHandler(
            std::shared_ptr<ClientStateManager> stateManager,
            std::shared_ptr<KeyManager> keyManager,
            std::shared_ptr<EventListener> eventListener
        );

        void handleAuthorizationResult(const nlohmann::json& jsonObject);
    
    private:
        std::shared_ptr<ClientStateManager> m_stateManager;
        std::shared_ptr<KeyManager> m_keyManager;
        std::shared_ptr<EventListener> m_eventListener;
    };
}