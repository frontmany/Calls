#pragma once

#include "clientStateManager.h"
#include "keyManager.h"
#include "packetType.h"
#include "userOperationManager.h"
#include "pendingRequests.h"
#include "network/packetHandler/packetFactory.h"
#include "protocol/packetType.h"
#include "protocol/errorCode.h"
#include "protocol/eventListener.h"
#include "network/TcpClient/client.h"
#include "network/UdpClient/client.h"
#include "json.hpp"
#include <functional>
#include <memory>
#include <atomic>
#include <thread>

namespace core
{
    namespace services
    {
        class AuthorizationService {
        public:
            AuthorizationService(
                ClientStateManager& stateManager,
                KeyManager& keyManager,
                std::shared_ptr<EventListener> eventListener,
                std::function<void(const std::vector<unsigned char>&, PacketType)> sendPacket
            );

            ~AuthorizationService();

            std::error_code authorize(const std::string& nickname);
            std::error_code logout();
            void handleReconnect();

            void onAuthorizeCompleted(const std::string& nickname, std::optional<nlohmann::json> completionContext);
            void onAuthorizeFailed(const std::string& nickname, std::optional<nlohmann::json> failureContext);

            void onLogoutFailed(std::optional<nlohmann::json> failureContext);
            void onReconnectCompleted(std::optional<nlohmann::json> completionContext);
            void onReconnectFailed(std::optional<nlohmann::json> failureContext);

            void startReconnectRetry();
            void stopReconnectRetry();
            void onConnectionDown();
            void onConnectionRestored();

        private:
            bool sendControl(uint32_t type, const std::vector<unsigned char>& body,
                std::function<void(std::optional<nlohmann::json>)> onComplete,
                std::function<void(std::optional<nlohmann::json>)> onFail,
                const std::string& uid);

            bool sendControlFireAndForget(uint32_t type, const std::vector<unsigned char>& body,
                std::function<void(std::optional<nlohmann::json>)> onComplete,
                std::function<void(std::optional<nlohmann::json>)> onFail,
                const std::string& uid);

        private:
            ClientStateManager& m_stateManager;
            KeyManager& m_keyManager;
            UserOperationManager& m_operationManager;
            PendingRequests& m_pendingRequests;
            core::network::udp::Client& m_mediaController;
            std::unique_ptr<core::network::tcp::Client>& m_controlController;
            std::shared_ptr<EventListener> m_eventListener;
            std::string m_host;
            std::string m_controlPort;

            std::atomic<bool> m_reconnectInProgress{false};
            std::thread m_reconnectRetryThread;
            std::atomic<bool> m_stopReconnectRetry{false};
            std::atomic<bool> m_retryThreadRunning{false};
        };
    }
}
