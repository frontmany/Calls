#pragma once

#include "logic/clientStateManager.h"
#include "eventListener.h"

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <thread>

namespace core::logic
{
    class ConnectionEstablishService {
    public:
        ConnectionEstablishService(
            std::shared_ptr<ClientStateManager> stateManager,
            std::function<bool()>&& attemptEstablishConnection,
            std::function<void()>&& onConnectionEstablished,
            std::chrono::milliseconds retryInterval = std::chrono::milliseconds(2000)
        );

        ~ConnectionEstablishService();

        void startConnectionAttempts();
        void stopConnectionAttempts();

    private:
        void connectionLoop();

    private:
        std::shared_ptr<ClientStateManager> m_stateManager;
        std::function<bool()> m_attemptEstablishConnection;
        std::function<void()> m_onConnectionEstablished;
        std::chrono::milliseconds m_retryInterval;
        std::thread m_thread;

        std::atomic<bool> m_running{ true };
        std::atomic<bool> m_attempting{ false };
    };
}
