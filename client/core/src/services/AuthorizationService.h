#pragma once

#include "IAuthorizationService.h"
#include "clientStateManager.h"
#include "keyManager.h"
#include "userOperationManager.h"
#include "pendingRequests.h"
#include "packetFactory.h"
#include "packetType.h"
#include "errorCode.h"
#include "eventListener.h"
#include "network/tcp/controlController.h"
#include "network/udp/mediaController.h"
#include "json.hpp"
#include <functional>
#include <memory>
#include <atomic>
#include <thread>

namespace core
{
    namespace services
    {
        class AuthorizationService : public IAuthorizationService {
        public:
            AuthorizationService(
                ClientStateManager& stateManager, 
                KeyManager& keyManager,
                UserOperationManager& operationManager,
                PendingRequests& pendingRequests,
                core::network::udp::MediaController& mediaController,
                std::unique_ptr<core::network::tcp::ControlController>& controlController,
                std::shared_ptr<EventListener> eventListener,
                std::string host,
                std::string controlPort
            );

            ~AuthorizationService();

            std::error_code authorize(const std::string& nickname) override;
            std::error_code logout() override;
            void handleReconnect() override;

            void onAuthorizeCompleted(const std::string& nickname, std::optional<nlohmann::json> completionContext);
            void onAuthorizeFailed(const std::string& nickname, std::optional<nlohmann::json> failureContext);
            void onLogoutCompleted(std::optional<nlohmann::json> completionContext);
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
            core::network::udp::MediaController& m_mediaController;
            std::unique_ptr<core::network::tcp::ControlController>& m_controlController;
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
