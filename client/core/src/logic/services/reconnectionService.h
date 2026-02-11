#pragma once

#include "logic/clientStateManager.h"
#include "logic/packetFactory.h"
#include "constants/packetType.h"
#include "eventListener.h"
#include "json.hpp"

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <thread>
#include <vector>

namespace core::logic
{
    class ReconnectionService {
    public:
        ReconnectionService(
           std::shared_ptr<ClientStateManager> stateManager,
           std::shared_ptr<EventListener> eventListener,
           std::function<uint16_t()>&& getLocalUdpPort,
           std::function<std::error_code(const std::vector<unsigned char>, constant::PacketType)>&& sendPacket,
           std::function<bool()>&& attemptEstablishConnection
        );

        ~ReconnectionService();

        void startReconnectionAttempts();
        void stopReconnectionAttempts();

    private:
        void reconnectLoop();
        void handleReconnect();

        void onReconnectCompleted(std::optional<nlohmann::json> completionContext);
        void onReconnectFailed(std::optional<nlohmann::json> failureContext);

    private:
        std::shared_ptr<ClientStateManager> m_stateManager;
        std::shared_ptr<EventListener> m_eventListener;
        uint16_t m_localUdpPort;
        std::thread m_thread;

        std::atomic<bool> m_running{ true};
        std::atomic<bool> m_reconnecting{ false};

        std::function<uint16_t()> m_getLocalUdpPort;
        std::function<bool()> m_attemptEstablishConnection; // blocking for n seconds
        std::function<std::error_code(const std::vector<unsigned char>, constant::PacketType)> m_sendPacket;
    };
}
