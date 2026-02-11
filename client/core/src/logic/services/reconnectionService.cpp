#include "reconnectionService.h"

#include "logic/packetFactory.h"
#include "utilities/logger.h"
#include "network/udp/client.h"
#include "constants/jsonType.h"

#include <chrono>
#include <thread>

using namespace core::constant;
using namespace core::utilities;
using namespace std::chrono_literals;

namespace core::logic
{
    ReconnectionService::ReconnectionService(
        std::shared_ptr<ClientStateManager> stateManager,
        std::shared_ptr<EventListener> eventListener,
        std::function<uint16_t()>&& getLocalUdpPort,
        std::function<std::error_code(const std::vector<unsigned char>, PacketType)>&& sendPacket,
        std::function<bool()>&& attemptEstablishConnection)
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
}
