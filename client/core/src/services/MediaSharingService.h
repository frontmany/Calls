#pragma once

#include "IMediaSharingService.h"
#include "IMediaEncryptionService.h"
#include "clientStateManager.h"
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

namespace core
{
    namespace services
    {
        class MediaSharingService : public IMediaSharingService {
        public:
            MediaSharingService(
                ClientStateManager& stateManager,
                UserOperationManager& operationManager,
                PendingRequests& pendingRequests,
            core::network::udp::MediaController& mediaController,
            std::unique_ptr<core::network::tcp::ControlController>& controlController,
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
            UserOperationManager& m_operationManager;
            PendingRequests& m_pendingRequests;
            core::network::udp::MediaController& m_mediaController;
            std::unique_ptr<core::network::tcp::ControlController>& m_controlController;
            IMediaEncryptionService& m_mediaEncryptionService;
            std::shared_ptr<EventListener> m_eventListener;
        };
    }
}
