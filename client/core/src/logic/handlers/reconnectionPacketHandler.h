#pragma once

#include <functional>
#include <memory>
#include <string>
#include "eventListener.h"

#include "json.hpp"

namespace core::logic
{
    class ClientStateManager;
    class KeyManager;

    class ReconnectionPacketHandler {
    public:
        ReconnectionPacketHandler(
            std::shared_ptr<ClientStateManager> stateManager,
            std::shared_ptr<EventListener> eventListener
        );

        void handleReconnectResult(const nlohmann::json& jsonObject);

    private:
        std::shared_ptr<ClientStateManager> m_stateManager;
        std::shared_ptr<EventListener> m_eventListener;
    };
}