#pragma once

#include "IMediaSharingService.h"
#include "IMediaEncryptionService.h"
#include "clientStateManager.h"
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

namespace core
{
    namespace services
    {
        // Сервис для управления screen/camera sharing
        class MediaSharingService : public IMediaSharingService {
        public:
            MediaSharingService(
                ClientStateManager& stateManager,
                UserOperationManager& operationManager,
                TaskManager<long long, std::milli>& taskManager,
                core::network::NetworkController& networkController,
                IMediaEncryptionService& mediaEncryptionService,
                std::shared_ptr<EventListener> eventListener
            );

            std::error_code startScreenSharing() override;
            std::error_code stopScreenSharing() override;
            std::error_code sendScreen(const std::vector<unsigned char>& data) override;
            std::error_code startCameraSharing() override;
            std::error_code stopCameraSharing() override;
            std::error_code sendCamera(const std::vector<unsigned char>& data) override;

            // Callbacks для обработки результатов операций
            void onStartScreenSharingCompleted(std::optional<nlohmann::json> completionContext);
            void onStartScreenSharingFailed(std::optional<nlohmann::json> failureContext);
            void onStopScreenSharingCompleted(std::optional<nlohmann::json> completionContext);
            void onStopScreenSharingFailed(std::optional<nlohmann::json> failureContext);
            void onStartCameraSharingCompleted(std::optional<nlohmann::json> completionContext);
            void onStartCameraSharingFailed(std::optional<nlohmann::json> failureContext);
            void onStopCameraSharingCompleted(std::optional<nlohmann::json> completionContext);
            void onStopCameraSharingFailed(std::optional<nlohmann::json> failureContext);

        private:
            void createAndStartTask(
                const std::string& uid,
                const std::vector<unsigned char>& packet,
                PacketType packetType,
                std::function<void(std::optional<nlohmann::json>)> onCompletion,
                std::function<void(std::optional<nlohmann::json>)> onFailure);

        private:
            ClientStateManager& m_stateManager;
            UserOperationManager& m_operationManager;
            TaskManager<long long, std::milli>& m_taskManager;
            core::network::NetworkController& m_networkController;
            IMediaEncryptionService& m_mediaEncryptionService;
            std::shared_ptr<EventListener> m_eventListener;
        };
    }
}
