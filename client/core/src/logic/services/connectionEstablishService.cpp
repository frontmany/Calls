#include "connectionEstablishService.h"
#include "utilities/logger.h"

#include <chrono>
#include <thread>

using namespace core::utilities;

namespace core::logic
{
    ConnectionEstablishService::ConnectionEstablishService(
        std::shared_ptr<ClientStateManager> stateManager,
        std::function<bool()>&& attemptEstablishConnection,
        std::function<void()>&& onConnectionEstablished,
        std::chrono::milliseconds retryInterval)
        : m_stateManager(std::move(stateManager))
        , m_attemptEstablishConnection(std::move(attemptEstablishConnection))
        , m_onConnectionEstablished(std::move(onConnectionEstablished))
        , m_retryInterval(retryInterval)
    {
        m_thread = std::thread([this]() {
            connectionLoop();
        });
    }

    ConnectionEstablishService::~ConnectionEstablishService()
    {
        stopConnectionAttempts();
        m_running = false;
        if (m_thread.joinable()) {
            m_thread.join();
        }
    }

    void ConnectionEstablishService::connectionLoop()
    {
        while (m_running) {
            if (m_attempting && m_stateManager && m_stateManager->isConnectionDown()) {
                bool connectionEstablished = m_attemptEstablishConnection();

                if (connectionEstablished) {
                    m_attempting = false;
                    m_onConnectionEstablished();
                }
                else {
                    std::this_thread::sleep_for(m_retryInterval);
                }
            }
            else {
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
        }
    }

    void ConnectionEstablishService::startConnectionAttempts()
    {
        m_attempting = true;
    }

    void ConnectionEstablishService::stopConnectionAttempts()
    {
        m_attempting = false;
    }
}
