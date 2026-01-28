#include "MediaSharingService.h"
#include "utilities/logger.h"
#include <chrono>

using namespace core::utilities;
using namespace std::chrono_literals;
using namespace std::placeholders;

namespace core
{
    namespace services
    {
        MediaSharingService::MediaSharingService(
            ClientStateManager& stateManager,
            UserOperationManager& operationManager,
            PendingRequests& pendingRequests,
            core::network::NetworkController& networkController,
            std::unique_ptr<core::network::TcpControlClient>& tcpControl,
            IMediaEncryptionService& mediaEncryptionService,
            std::shared_ptr<EventListener> eventListener)
            : m_stateManager(stateManager)
            , m_operationManager(operationManager)
            , m_pendingRequests(pendingRequests)
            , m_networkController(networkController)
            , m_tcpControl(tcpControl)
            , m_mediaEncryptionService(mediaEncryptionService)
            , m_eventListener(eventListener)
        {
        }

        bool MediaSharingService::sendControl(uint32_t type, const std::vector<unsigned char>& body,
            std::function<void(std::optional<nlohmann::json>)> onComplete,
            std::function<void(std::optional<nlohmann::json>)> onFail,
            const std::string& uid)
        {
            if (!m_tcpControl || !m_tcpControl->isConnected()) return false;
            m_pendingRequests.add(uid, std::move(onComplete), std::move(onFail));
            if (!m_tcpControl->send(type, body.data(), body.size())) {
                m_pendingRequests.fail(uid, std::nullopt);
                return false;
            }
            return true;
        }

        bool MediaSharingService::sendControlFireAndForget(uint32_t type, const std::vector<unsigned char>& body,
            std::function<void(std::optional<nlohmann::json>)> onComplete,
            std::function<void(std::optional<nlohmann::json>)> onFail,
            const std::string& uid)
        {
            if (!sendControl(type, body, std::move(onComplete), std::move(onFail), uid))
                return false;
            m_pendingRequests.complete(uid, std::nullopt);
            return true;
        }

        std::error_code MediaSharingService::startScreenSharing() {
            if (m_stateManager.isConnectionDown()) return make_error_code(ErrorCode::connection_down);
            if (!m_stateManager.isAuthorized()) return make_error_code(ErrorCode::not_authorized);
            if (!m_stateManager.isActiveCall()) return make_error_code(ErrorCode::no_active_call);
            if (m_stateManager.isScreenSharing()) return make_error_code(ErrorCode::screen_sharing_already_active);
            if (m_stateManager.isViewingRemoteScreen()) return make_error_code(ErrorCode::viewing_remote_screen);
            const std::string& friendNickname = m_stateManager.getActiveCall().getNickname();
            if (m_operationManager.isOperation(UserOperationType::START_SCREEN_SHARING, friendNickname)) return make_error_code(ErrorCode::operation_in_progress);

            m_operationManager.addOperation(UserOperationType::START_SCREEN_SHARING, friendNickname);
            auto [uid, packet] = PacketFactory::getStartScreenSharingPacket(m_stateManager.getMyNickname(), friendNickname);

            if (!sendControlFireAndForget(static_cast<uint32_t>(PacketType::SCREEN_SHARING_BEGIN), packet,
                    std::bind(&MediaSharingService::onStartScreenSharingCompleted, this, _1),
                    std::bind(&MediaSharingService::onStartScreenSharingFailed, this, _1), uid)) {
                m_operationManager.removeOperation(UserOperationType::START_SCREEN_SHARING, friendNickname);
                return make_error_code(ErrorCode::connection_down);
            }
            return {};
        }

        std::error_code MediaSharingService::stopScreenSharing() {
            if (m_stateManager.isConnectionDown()) return make_error_code(ErrorCode::connection_down);
            if (!m_stateManager.isAuthorized()) return make_error_code(ErrorCode::not_authorized);
            if (!m_stateManager.isActiveCall()) return make_error_code(ErrorCode::no_active_call);
            if (!m_stateManager.isScreenSharing()) return make_error_code(ErrorCode::screen_sharing_not_active);
            const std::string& friendNickname = m_stateManager.getActiveCall().getNickname();
            if (m_operationManager.isOperation(UserOperationType::STOP_SCREEN_SHARING, friendNickname)) return make_error_code(ErrorCode::operation_in_progress);

            m_operationManager.addOperation(UserOperationType::STOP_SCREEN_SHARING, friendNickname);
            auto [uid, packet] = PacketFactory::getStopScreenSharingPacket(m_stateManager.getMyNickname(), friendNickname);

            if (!sendControlFireAndForget(static_cast<uint32_t>(PacketType::SCREEN_SHARING_END), packet,
                    std::bind(&MediaSharingService::onStopScreenSharingCompleted, this, _1),
                    std::bind(&MediaSharingService::onStopScreenSharingFailed, this, _1), uid)) {
                m_operationManager.removeOperation(UserOperationType::STOP_SCREEN_SHARING, friendNickname);
                return make_error_code(ErrorCode::connection_down);
            }
            return {};
        }

        std::error_code MediaSharingService::sendScreen(const std::vector<unsigned char>& data) {
            if (m_stateManager.isConnectionDown()) return make_error_code(ErrorCode::connection_down);
            if (!m_stateManager.isAuthorized()) return make_error_code(ErrorCode::not_authorized);
            if (!m_stateManager.isActiveCall()) return make_error_code(ErrorCode::no_active_call);
            if (!m_stateManager.isScreenSharing()) return make_error_code(ErrorCode::screen_sharing_not_active);

            const CryptoPP::SecByteBlock& callKey = m_stateManager.getActiveCall().getCallKey();

            auto cipherData = m_mediaEncryptionService.encryptMedia(
                data.data(),
                static_cast<int>(data.size()),
                callKey);

            if (cipherData.empty()) {
                LOG_ERROR("Screen sending error: encryption failed");
                return make_error_code(ErrorCode::encryption_error);
            }

            m_networkController.send(std::move(cipherData), static_cast<uint32_t>(PacketType::SCREEN));
            return {};
        }

        std::error_code MediaSharingService::startCameraSharing() {
            if (m_stateManager.isConnectionDown()) return make_error_code(ErrorCode::connection_down);
            if (!m_stateManager.isAuthorized()) return make_error_code(ErrorCode::not_authorized);
            if (!m_stateManager.isActiveCall()) return make_error_code(ErrorCode::no_active_call);
            if (m_stateManager.isCameraSharing()) return make_error_code(ErrorCode::camera_sharing_already_active);
            const std::string& friendNickname = m_stateManager.getActiveCall().getNickname();
            if (m_operationManager.isOperation(UserOperationType::START_CAMERA_SHARING, friendNickname)) return make_error_code(ErrorCode::operation_in_progress);

            m_operationManager.addOperation(UserOperationType::START_CAMERA_SHARING, friendNickname);
            auto [uid, packet] = PacketFactory::getStartCameraSharingPacket(m_stateManager.getMyNickname(), friendNickname);

            if (!sendControlFireAndForget(static_cast<uint32_t>(PacketType::CAMERA_SHARING_BEGIN), packet,
                    std::bind(&MediaSharingService::onStartCameraSharingCompleted, this, _1),
                    std::bind(&MediaSharingService::onStartCameraSharingFailed, this, _1), uid)) {
                m_operationManager.removeOperation(UserOperationType::START_CAMERA_SHARING, friendNickname);
                return make_error_code(ErrorCode::connection_down);
            }
            return {};
        }

        std::error_code MediaSharingService::stopCameraSharing() {
            if (m_stateManager.isConnectionDown()) return make_error_code(ErrorCode::connection_down);
            if (!m_stateManager.isAuthorized()) return make_error_code(ErrorCode::not_authorized);
            if (!m_stateManager.isActiveCall()) return make_error_code(ErrorCode::no_active_call);
            if (!m_stateManager.isCameraSharing()) return make_error_code(ErrorCode::camera_sharing_not_active);
            const std::string& friendNickname = m_stateManager.getActiveCall().getNickname();
            if (m_operationManager.isOperation(UserOperationType::STOP_CAMERA_SHARING, friendNickname)) return make_error_code(ErrorCode::operation_in_progress);

            m_operationManager.addOperation(UserOperationType::STOP_CAMERA_SHARING, friendNickname);
            auto [uid, packet] = PacketFactory::getStopCameraSharingPacket(m_stateManager.getMyNickname(), friendNickname);

            if (!sendControlFireAndForget(static_cast<uint32_t>(PacketType::CAMERA_SHARING_END), packet,
                    std::bind(&MediaSharingService::onStopCameraSharingCompleted, this, _1),
                    std::bind(&MediaSharingService::onStopCameraSharingFailed, this, _1), uid)) {
                m_operationManager.removeOperation(UserOperationType::STOP_CAMERA_SHARING, friendNickname);
                return make_error_code(ErrorCode::connection_down);
            }
            return {};
        }

        std::error_code MediaSharingService::sendCamera(const std::vector<unsigned char>& data) {
            if (m_stateManager.isConnectionDown()) return make_error_code(ErrorCode::connection_down);
            if (!m_stateManager.isAuthorized()) return make_error_code(ErrorCode::not_authorized);
            if (!m_stateManager.isActiveCall()) return make_error_code(ErrorCode::no_active_call);
            if (!m_stateManager.isCameraSharing()) return make_error_code(ErrorCode::camera_sharing_not_active);

            const CryptoPP::SecByteBlock& callKey = m_stateManager.getActiveCall().getCallKey();

            auto cipherData = m_mediaEncryptionService.encryptMedia(
                data.data(),
                static_cast<int>(data.size()),
                callKey);

            if (cipherData.empty()) {
                LOG_ERROR("Camera sending error: encryption failed");
                return make_error_code(ErrorCode::encryption_error);
            }

            m_networkController.send(std::move(cipherData), static_cast<uint32_t>(PacketType::CAMERA));
            return {};
        }

        void MediaSharingService::onStartScreenSharingCompleted(std::optional<nlohmann::json> completionContext) {
            const std::string& friendNickname = m_stateManager.getActiveCall().getNickname();
            m_operationManager.removeOperation(UserOperationType::START_SCREEN_SHARING, friendNickname);
            m_stateManager.setScreenSharing(true);
            m_eventListener->onStartScreenSharingResult({});
        }

        void MediaSharingService::onStartScreenSharingFailed(std::optional<nlohmann::json> failureContext) {
            const std::string& friendNickname = m_stateManager.getActiveCall().getNickname();
            m_operationManager.removeOperation(UserOperationType::START_SCREEN_SHARING, friendNickname);
            LOG_ERROR("Start screen sharing task failed");
            m_eventListener->onStartScreenSharingResult(ErrorCode::network_error);
        }

        void MediaSharingService::onStopScreenSharingCompleted(std::optional<nlohmann::json> completionContext) {
            const std::string& friendNickname = m_stateManager.getActiveCall().getNickname();
            m_operationManager.removeOperation(UserOperationType::STOP_SCREEN_SHARING, friendNickname);
            m_stateManager.setScreenSharing(false);
            m_eventListener->onStopScreenSharingResult({});
        }

        void MediaSharingService::onStopScreenSharingFailed(std::optional<nlohmann::json> failureContext) {
            const std::string& friendNickname = m_stateManager.getActiveCall().getNickname();
            m_operationManager.removeOperation(UserOperationType::STOP_SCREEN_SHARING, friendNickname);
            LOG_ERROR("Stop screen sharing task failed");
            m_eventListener->onStopScreenSharingResult(ErrorCode::network_error);
        }

        void MediaSharingService::onStartCameraSharingCompleted(std::optional<nlohmann::json> completionContext) {
            const std::string& friendNickname = m_stateManager.getActiveCall().getNickname();
            m_operationManager.removeOperation(UserOperationType::START_CAMERA_SHARING, friendNickname);
            m_stateManager.setCameraSharing(true);
            m_eventListener->onStartCameraSharingResult({});
        }

        void MediaSharingService::onStartCameraSharingFailed(std::optional<nlohmann::json> failureContext) {
            const std::string& friendNickname = m_stateManager.getActiveCall().getNickname();
            m_operationManager.removeOperation(UserOperationType::START_CAMERA_SHARING, friendNickname);
            LOG_ERROR("Start camera sharing task failed");
            m_eventListener->onStartCameraSharingResult(ErrorCode::network_error);
        }

        void MediaSharingService::onStopCameraSharingCompleted(std::optional<nlohmann::json> completionContext) {
            const std::string& friendNickname = m_stateManager.getActiveCall().getNickname();
            m_operationManager.removeOperation(UserOperationType::STOP_CAMERA_SHARING, friendNickname);
            m_stateManager.setCameraSharing(false);
            m_eventListener->onStopCameraSharingResult({});
        }

        void MediaSharingService::onStopCameraSharingFailed(std::optional<nlohmann::json> failureContext) {
            const std::string& friendNickname = m_stateManager.getActiveCall().getNickname();
            m_operationManager.removeOperation(UserOperationType::STOP_CAMERA_SHARING, friendNickname);
            LOG_ERROR("Stop camera sharing task failed");
            m_eventListener->onStopCameraSharingResult(ErrorCode::network_error);
        }
    }
}
