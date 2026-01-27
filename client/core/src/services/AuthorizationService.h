#pragma once

#include "IAuthorizationService.h"
#include "clientStateManager.h"
#include "keyManager.h"
#include "userOperationManager.h"
#include "taskManager.h"
#include "packetFactory.h"
#include "packetType.h"
#include "errorCode.h"
#include "eventListener.h"
#include "network/networkController.h"
#include "json.hpp"
#include <functional>
#include <memory>
#include <atomic>
#include <thread>

namespace core
{
    namespace services
    {
        // Сервис для управления авторизацией и переподключением
        class AuthorizationService : public IAuthorizationService {
        public:
            AuthorizationService(
                ClientStateManager& stateManager,
                KeyManager& keyManager,
                UserOperationManager& operationManager,
                TaskManager<long long, std::milli>& taskManager,
                core::network::NetworkController& networkController,
                std::shared_ptr<EventListener> eventListener
            );

            ~AuthorizationService();

            std::error_code authorize(const std::string& nickname) override;
            std::error_code logout() override;
            void handleReconnect() override;

            // Callbacks для обработки результатов операций
            void onAuthorizeCompleted(const std::string& nickname, std::optional<nlohmann::json> completionContext);
            void onAuthorizeFailed(const std::string& nickname, std::optional<nlohmann::json> failureContext);
            void onLogoutCompleted(std::optional<nlohmann::json> completionContext);
            void onLogoutFailed(std::optional<nlohmann::json> failureContext);
            void onReconnectCompleted(std::optional<nlohmann::json> completionContext);
            void onReconnectFailed(std::optional<nlohmann::json> failureContext);

            // Методы для управления переподключением
            void startReconnectRetry();
            void stopReconnectRetry();
            void onConnectionDown();
            void onConnectionRestored();

        private:
            void createAndStartTask(
                const std::string& uid,
                const std::vector<unsigned char>& packet,
                PacketType packetType,
                std::function<void(std::optional<nlohmann::json>)> onCompletion,
                std::function<void(std::optional<nlohmann::json>)> onFailure);

        private:
            ClientStateManager& m_stateManager;
            KeyManager& m_keyManager;
            UserOperationManager& m_operationManager;
            TaskManager<long long, std::milli>& m_taskManager;
            core::network::NetworkController& m_networkController;
            std::shared_ptr<EventListener> m_eventListener;

            std::atomic<bool> m_reconnectInProgress{false};
            std::thread m_reconnectRetryThread;
            std::atomic<bool> m_stopReconnectRetry{false};
        };
    }
}
