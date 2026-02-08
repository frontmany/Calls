#include "reconnectionService.h"

#include "logic/packetFactory.h"
#include "utilities/logger.h"
#include "network/udp/client.h"

#include <chrono>
#include <thread>

using namespace core::utilities;
using namespace std::chrono_literals;

namespace core::logic
{
    ReconnectionService::ReconnectionService(
        std::shared_ptr<ClientStateManager> stateManager,
        std::shared_ptr<EventListener> eventListener,
        std::function<uint16_t()>&& getLocalUdpPort,
        std::function<bool()>&& attemptEstablishConnection,
        std::function<std::error_code(const std::vector<unsigned char>, PacketType)>&& sendPacket)
        : m_stateManager(stateManager)
        , m_eventListener(eventListener)
        , m_getLocalUdpPort(std::move(getLocalUdpPort))
        , m_attemptEstablishConnection(std::move(attemptEstablishConnection))
        , m_sendPacket(std::move(sendPacket))
    {
        m_thread = std::thread([this]() {
            reconnectLoop();
        });
    }

    ReconnectionService::~ReconnectionService()
    {
        stopReconnectionAttempts();
    }

    void ReconnectionService::reconnectLoop() {
        while (m_running) {
            if (m_reconnecting) {
                if (m_stateManager->isConnectionDown()) {
                    bool connectionEstablished = m_attemptEstablishConnection();

                    if (connectionEstablished) {
                        handleReconnect();
                    }
                }
            }
            else {
                std::this_thread::sleep_for(1s);
            }
        }
    }

    void ReconnectionService::startReconnectionAttempts() {
        m_reconnecting.store(true);
    }

    void ReconnectionService::stopReconnectionAttempts() {
        m_reconnecting.store(false);
    }

    void ReconnectionService::handleReconnect()
    {
        m_reconnecting.store(false);

        if (m_stateManager->isAuthorized()) {

            auto packet = PacketFactory::getReconnectPacket(
                m_stateManager->getMyNickname(),
                m_stateManager->getMyToken(),
                m_getLocalUdpPort()
            );

            if (m_sendPacket)
                m_sendPacket(packet, PacketType::RECONNECT);
        }
        else {
            m_stateManager->setConnectionDown(false);

            if (m_eventListener)
                m_eventListener->onConnectionRestoredAuthorizationNeeded();
        }
    }


    // those functions have to be moved to appropriate handler in future
    void ReconnectionService::onReconnectCompleted(std::optional<nlohmann::json> completionContext)
    {
        m_reconnectInProgress = false;

        if (!completionContext.has_value()) {
            LOG_ERROR("onReconnectCompleted empty");
            return;
        }

        auto& context = completionContext.value();
        m_stateManager.setConnectionDown(false);
        m_mediaController.notifyConnectionRestored();

        if (!context.contains(RESULT)) {
            m_stateManager.setAuthorized(false);
            m_stateManager.clearMyNickname();
            m_stateManager.clearMyToken();
            if (m_eventListener)
                m_eventListener->onConnectionRestoredAuthorizationNeeded();
            return;
        }

        bool ok = context[RESULT].get<bool>();
        if (ok) {
            LOG_INFO("Reconnect OK");
            m_stateManager.setLastReconnectSuccessTime();
            bool activeCall = context.contains(IS_ACTIVE_CALL) && context[IS_ACTIVE_CALL].get<bool>();
            if (m_eventListener)
                m_eventListener->onConnectionRestored();
            if (!activeCall) {
                bool had = m_stateManager.isActiveCall();
                m_stateManager.setScreenSharing(false);
                m_stateManager.setCameraSharing(false);
                m_stateManager.setViewingRemoteScreen(false);
                m_stateManager.setViewingRemoteCamera(false);
                m_stateManager.clearCallState();
                if (had && m_eventListener)
                    m_eventListener->onCallEndedByRemote({});
            }
        }
        else {
            m_stateManager.setAuthorized(false);
            m_stateManager.clearMyNickname();
            m_stateManager.clearMyToken();
            if (m_eventListener)
                m_eventListener->onConnectionRestoredAuthorizationNeeded();
        }
    }

    void ReconnectionService::onReconnectFailed(std::optional<nlohmann::json>)
    {
        m_reconnectInProgress = false;
        LOG_ERROR("Reconnect failed");
    }
}
