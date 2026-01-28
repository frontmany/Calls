#include "core.h"
#include "jsonType.h"
#include "packetFactory.h"
#include "utilities/crypto.h"
#include "json.hpp"

#include "utilities/logger.h"
#include "utilities/crashCatchInitializer.h"

#include <filesystem>

using namespace core::utilities;
using namespace std::chrono_literals;
using namespace std::placeholders;

namespace core
{
    void initializeDiagnostics(const std::string& appDirectory,
        const std::string& logDirectory,
        const std::string& crashDumpDirectory,
        const std::string& appVersion)
    {
        std::filesystem::path basePath = appDirectory.empty()
            ? std::filesystem::current_path()
            : std::filesystem::path(appDirectory);

        basePath = std::filesystem::absolute(basePath);

        std::filesystem::path logDir = logDirectory.empty()
            ? basePath / "logs"
            : std::filesystem::path(logDirectory);
        if (logDir.is_relative()) {
            logDir = basePath / logDir;
        }

        core::utilities::log::set_log_directory(logDir.string());
        std::filesystem::path crashDir = crashDumpDirectory.empty()
            ? basePath / "crashDumps"
            : std::filesystem::path(crashDumpDirectory);
        if (crashDir.is_relative()) {
            crashDir = basePath / crashDir;
        }

        core::utilities::initializeCrashCatch((crashDir / "calliforniaCore").string(),
            appVersion);
    }

    Client::Client()
    {
    }

    Client::~Client() {
        if (m_authorizationService) {
            m_authorizationService->stopReconnectRetry();
        }
        reset();
        if (m_controlController)
            m_controlController->disconnect();
        m_mediaController.stop();
    }

    bool Client::start(const std::string& host,
        const std::string& controlPort,
        const std::string& mediaPort,
        std::shared_ptr<EventListener> eventListener)
    {
        m_eventListener = std::move(eventListener);

        bool audioInitialized = m_audioEngine.init(std::bind(&Client::onInputVoice, this, _1, _2));

        bool mediaInitialized = m_mediaController.initialize(host, mediaPort,
            [this](const unsigned char* data, int length, uint32_t type) {
                auto packetType = static_cast<PacketType>(type);
                if (packetType == PacketType::VOICE || packetType == PacketType::SCREEN || packetType == PacketType::CAMERA)
                    onReceive(data, length, packetType);
            },
            std::bind(&Client::onConnectionDown, this),
            std::bind(&Client::onConnectionRestored, this)
        );

        m_controlController = std::make_unique<core::network::tcp::ControlController>(
            [this](uint32_t type, const unsigned char* data, size_t size) {
                if (m_packetProcessor)
                    m_packetProcessor->processPacket(data, static_cast<int>(size), static_cast<PacketType>(type));
            },
            [this]() { onConnectionDown(); });

        m_authorizationService = std::make_unique<core::services::AuthorizationService>(
            m_stateManager, m_keyManager, m_operationManager, m_pendingRequests,
            m_mediaController, m_controlController, m_eventListener, host, controlPort);
        m_callService = std::make_unique<core::services::CallService>(
            m_stateManager, m_keyManager, m_operationManager, m_pendingRequests,
            m_mediaController, m_controlController, m_audioEngine, m_eventListener);
        m_mediaSharingService = std::make_unique<core::services::MediaSharingService>(
            m_stateManager, m_operationManager, m_pendingRequests,
            m_mediaController, m_controlController, m_mediaEncryptionService, m_eventListener);
        m_packetProcessor = std::make_unique<PacketProcessor>(m_stateManager, m_keyManager, m_pendingRequests, m_mediaController, m_controlController, m_audioEngine, m_eventListener, m_mediaEncryptionService);
        m_packetProcessor->setOrphanReconnectSuccessHandler([this](std::optional<nlohmann::json> completionContext) { onReconnectCompleted(completionContext); });

        m_controlController->connect(host, controlPort);
        m_keyManager.generateKeys();

        if (mediaInitialized && !m_mediaController.isRunning())
            m_mediaController.start();

        if (audioInitialized && mediaInitialized) {
            return true;
        }
        LOG_ERROR("Calls client initialization failed - audio: {}, media: {}", audioInitialized, mediaInitialized);
        return false;
    }

    void Client::stop() {
        if (m_authorizationService) {
            m_authorizationService->stopReconnectRetry();
        }
        reset();
        if (m_controlController)
            m_controlController->disconnect();
        m_mediaController.stop();
    }

    void Client::reset() {
        m_stateManager.reset();
        m_keyManager.resetKeys();
        m_pendingRequests.failAll(std::nullopt);
        m_operationManager.clearAllOperations();

        if (m_audioEngine.isStream())
            m_audioEngine.stopStream();
    }

    void Client::onNetworkReceive(const unsigned char* data, int length, uint32_t type) {
        (void)data;
        (void)length;
        (void)type;
    }


    void Client::onConnectionDown() {
        LOG_ERROR("Connection down");
        m_pendingRequests.failAll(std::nullopt);

        m_stateManager.setScreenSharing(false);
        m_stateManager.setCameraSharing(false);
        m_stateManager.setViewingRemoteScreen(false);
        m_stateManager.setViewingRemoteCamera(false);

        if (m_stateManager.isOutgoingCall()) {
            m_stateManager.clearCallState();
            m_eventListener->onOutgoingCallTimeout(ErrorCode::network_error);
        }

        if (m_stateManager.isIncomingCalls()) {
            auto& incomingCalls = m_stateManager.getIncomingCalls();

            for (auto& [nickname, incomingCall] : incomingCalls) {
                m_eventListener->onIncomingCallExpired(ErrorCode::network_error, nickname);
            }

            m_stateManager.clearIncomingCalls();
        }

        if (m_stateManager.isActiveCall()) {
            m_audioEngine.stopStream();
        }

        m_eventListener->onConnectionDown();

        if (m_authorizationService) {
            m_authorizationService->onConnectionDown();
        }
    }

    void Client::onConnectionRestored() {
        if (m_authorizationService) {
            m_authorizationService->onConnectionRestored();
        }
    }

    void Client::onReconnectCompleted(std::optional<nlohmann::json> completionContext) {
        if (m_authorizationService) {
            m_authorizationService->onReconnectCompleted(completionContext);
        }
        // Если переподключение не удалось, нужно сбросить состояние
        if (completionContext.has_value()) {
            auto& context = completionContext.value();
            if (context.contains(RESULT) && !context[RESULT].get<bool>()) {
                reset();
            }
            else if (!context.contains(RESULT)) {
                reset();
            }
            else {
                bool activeCall = context.contains(IS_ACTIVE_CALL) ? context[IS_ACTIVE_CALL].get<bool>() : false;
                if (!activeCall && m_stateManager.isActiveCall()) {
                    m_audioEngine.stopStream();
                }
            }
        }
        else {
            // Если completionContext пустой, сбросить состояние
            reset();
        }
    }

    void Client::onReconnectFailed(std::optional<nlohmann::json> failureContext) {
        if (m_authorizationService) {
            m_authorizationService->onReconnectFailed(failureContext);
        }
    }

    void Client::onAuthorizeCompleted(const std::string& nickname, std::optional<nlohmann::json> completionContext) {
        if (m_authorizationService) {
            m_authorizationService->onAuthorizeCompleted(nickname, completionContext);
        }
    }

    void Client::onAuthorizeFailed(const std::string& nickname, std::optional<nlohmann::json> failureContext) {
        if (m_authorizationService) {
            m_authorizationService->onAuthorizeFailed(nickname, failureContext);
        }
    }

    void Client::onLogoutCompleted(std::optional<nlohmann::json> completionContext) {
        if (m_authorizationService) {
            m_authorizationService->onLogoutCompleted(completionContext);
        }
        // Reset вызывается после того, как UI обработает событие onLogoutCompleted
        // Это гарантирует, что состояние не изменится до закрытия приложения
    }

    void Client::onLogoutFailed(std::optional<nlohmann::json> failureContext) {
        if (m_authorizationService) {
            m_authorizationService->onLogoutFailed(failureContext);
        }
        // Reset вызывается после того, как UI обработает событие onLogoutCompleted
        // Это гарантирует, что состояние не изменится до закрытия приложения
    }

    void Client::onRequestUserInfoCompleted(const std::string& userNickname, std::optional<nlohmann::json> completionContext) {
        if (m_callService) {
            m_callService->onRequestUserInfoCompleted(userNickname, completionContext);
        }
    }

    void Client::onRequestUserInfoFailed(const std::string& userNickname, std::optional<nlohmann::json> failureContext) {
        if (m_callService) {
            m_callService->onRequestUserInfoFailed(userNickname, failureContext);
        }
    }

    void Client::onStartOutgoingCallCompleted(const std::string& userNickname, std::optional<nlohmann::json> completionContext) {
        if (m_callService) {
            m_callService->onStartOutgoingCallCompleted(userNickname, completionContext);
        }
    }

    void Client::onStartOutgoingCallFailed(const std::string& userNickname, std::optional<nlohmann::json> failureContext) {
        if (m_callService) {
            m_callService->onStartOutgoingCallFailed(userNickname, failureContext);
        }
    }

    void Client::onOutgoingCallTimeout() {
        if (m_callService) {
            m_callService->onOutgoingCallTimeout();
        }
    }

    void Client::onStopOutgoingCallCompleted(const std::string& nickname, std::optional<nlohmann::json> completionContext) {
        if (m_callService) {
            m_callService->onStopOutgoingCallCompleted(nickname, completionContext);
        }
    }

    void Client::onStopOutgoingCallFailed(const std::string& nickname, std::optional<nlohmann::json> failureContext) {
        if (m_callService) {
            m_callService->onStopOutgoingCallFailed(nickname, failureContext);
        }
    }

    void Client::onDeclineIncomingCallCompleted(const std::string& nickname, std::optional<nlohmann::json> completionContext) {
        if (m_callService) {
            m_callService->onDeclineIncomingCallCompleted(nickname, completionContext);
        }
    }

    void Client::onDeclineIncomingCallFailed(std::optional<nlohmann::json> failureContext) {
        if (m_callService) {
            m_callService->onDeclineIncomingCallFailed(failureContext);
        }
    }

    void Client::onAcceptCallStopOutgoingCompleted(const std::string& userNickname, std::optional<nlohmann::json> completionContext) {
        if (m_callService) {
            m_callService->onAcceptCallStopOutgoingCompleted(userNickname, completionContext);
        }
    }

    void Client::onAcceptCallStopOutgoingFailed(const std::string& userNickname, std::optional<nlohmann::json> failureContext) {
        if (m_callService) {
            m_callService->onAcceptCallStopOutgoingFailed(userNickname, failureContext);
        }
    }

    void Client::onAcceptCallEndActiveCompleted(const std::string& userNickname, std::optional<nlohmann::json> completionContext) {
        if (m_callService) {
            m_callService->onAcceptCallEndActiveCompleted(userNickname, completionContext);
        }
    }

    void Client::onAcceptCallEndActiveFailed(const std::string& userNickname, std::optional<nlohmann::json> failureContext) {
        if (m_callService) {
            m_callService->onAcceptCallEndActiveFailed(userNickname, failureContext);
        }
    }

    void Client::onAcceptCallCompleted(const std::string& userNickname, std::optional<nlohmann::json> completionContext) {
        if (m_callService) {
            m_callService->onAcceptCallCompleted(userNickname, completionContext);
        }
    }

    void Client::onAcceptCallFailed(const std::string& userNickname, std::optional<nlohmann::json> failureContext) {
        if (m_callService) {
            m_callService->onAcceptCallFailed(userNickname, failureContext);
        }
    }

    void Client::onAcceptCallFailedAfterEndCall(const std::string& userNickname, std::optional<nlohmann::json> failureContext) {
        if (m_callService) {
            m_callService->onAcceptCallFailedAfterEndCall(userNickname, failureContext);
        }
    }

    void Client::onDeclineCallCompleted(const std::string& userNickname, std::optional<nlohmann::json> completionContext) {
        if (m_callService) {
            m_callService->onDeclineCallCompleted(userNickname, completionContext);
        }
    }

    void Client::onDeclineCallFailed(const std::string& userNickname, std::optional<nlohmann::json> failureContext) {
        if (m_callService) {
            m_callService->onDeclineCallFailed(userNickname, failureContext);
        }
    }

    void Client::onEndCallCompleted(const std::string& nickname, std::optional<nlohmann::json> completionContext) {
        if (m_callService) {
            m_callService->onEndCallCompleted(nickname, completionContext);
        }
    }

    void Client::onEndCallFailed(const std::string& nickname, std::optional<nlohmann::json> failureContext) {
        if (m_callService) {
            m_callService->onEndCallFailed(nickname, failureContext);
        }
    }

    void Client::onStartScreenSharingCompleted(std::optional<nlohmann::json> completionContext) {
        if (m_mediaSharingService) {
            m_mediaSharingService->onStartScreenSharingCompleted(completionContext);
        }
    }

    void Client::onStartScreenSharingFailed(std::optional<nlohmann::json> failureContext) {
        if (m_mediaSharingService) {
            m_mediaSharingService->onStartScreenSharingFailed(failureContext);
        }
    }

    void Client::onStopScreenSharingCompleted(std::optional<nlohmann::json> completionContext) {
        if (m_mediaSharingService) {
            m_mediaSharingService->onStopScreenSharingCompleted(completionContext);
        }
    }

    void Client::onStopScreenSharingFailed(std::optional<nlohmann::json> failureContext) {
        if (m_mediaSharingService) {
            m_mediaSharingService->onStopScreenSharingFailed(failureContext);
        }
    }

    void Client::onStartCameraSharingCompleted(std::optional<nlohmann::json> completionContext) {
        if (m_mediaSharingService) {
            m_mediaSharingService->onStartCameraSharingCompleted(completionContext);
        }
    }

    void Client::onStartCameraSharingFailed(std::optional<nlohmann::json> failureContext) {
        if (m_mediaSharingService) {
            m_mediaSharingService->onStartCameraSharingFailed(failureContext);
        }
    }

    void Client::onStopCameraSharingCompleted(std::optional<nlohmann::json> completionContext) {
        if (m_mediaSharingService) {
            m_mediaSharingService->onStopCameraSharingCompleted(completionContext);
        }
    }

    void Client::onStopCameraSharingFailed(std::optional<nlohmann::json> failureContext) {
        if (m_mediaSharingService) {
            m_mediaSharingService->onStopCameraSharingFailed(failureContext);
        }
    }

    void Client::onReceive(const unsigned char* data, int length, PacketType type) {
        if (m_packetProcessor) {
            m_packetProcessor->processPacket(data, length, type);
        }
    }

    std::vector<std::string> Client::getCallers() const {
        auto& incomingCalls = m_stateManager.getIncomingCalls();

        std::vector<std::string> callersNicknames;
        for (auto& [nickname, incomingCall] : incomingCalls) {
            callersNicknames.push_back(nickname);
        }

        return callersNicknames;
    }

    void Client::muteMicrophone(bool isMute) {
        m_audioEngine.muteMicrophone(isMute);
    }

    void Client::muteSpeaker(bool isMute) {
        m_audioEngine.muteSpeaker(isMute);
    }

    bool Client::isScreenSharing() const {
        return m_stateManager.isScreenSharing();
    }

    bool Client::isViewingRemoteScreen() const {
        return m_stateManager.isViewingRemoteScreen();
    }

    bool Client::isCameraSharing() const {
        return m_stateManager.isCameraSharing();
    }

    bool Client::isViewingRemoteCamera() const {
        return m_stateManager.isViewingRemoteCamera();
    }

    bool Client::isMicrophoneMuted() const {
        return m_audioEngine.isMicrophoneMuted();
    }

    bool Client::isSpeakerMuted() const {
        return m_audioEngine.isSpeakerMuted();
    }

    void Client::refreshAudioDevices() {
        m_audioEngine.refreshAudioDevices();
    }

    int Client::getInputVolume() const {
        return m_audioEngine.getInputVolume();
    }

    int Client::getOutputVolume() const {
        return m_audioEngine.getOutputVolume();
    }

    void Client::setInputVolume(int volume) {
        m_audioEngine.setInputVolume(volume);
    }

    void Client::setOutputVolume(int volume) {
        m_audioEngine.setOutputVolume(volume);
    }

    bool Client::setInputDevice(int deviceIndex) {
        return m_audioEngine.setInputDevice(deviceIndex);
    }

    bool Client::setOutputDevice(int deviceIndex) {
        return m_audioEngine.setOutputDevice(deviceIndex);
    }

    int Client::getCurrentInputDevice() const {
        return m_audioEngine.getCurrentInputDevice();
    }

    int Client::getCurrentOutputDevice() const {
        return m_audioEngine.getCurrentOutputDevice();
    }

    bool Client::isAuthorized() const {
        return m_stateManager.isAuthorized();
    }

    bool Client::isOutgoingCall() const {
        return m_stateManager.isOutgoingCall();
    }

    bool Client::isActiveCall() const {
        return m_stateManager.isActiveCall();
    }

    bool Client::isConnectionDown() const {
        return m_stateManager.isConnectionDown();
    }

    int Client::getIncomingCallsCount() const {
        return m_stateManager.getIncomingCallsCount();
    }

    const std::string& Client::getMyNickname() const {
        return m_stateManager.getMyNickname();
    }

    const std::string& Client::getNicknameWhomCalling() const {
        return m_stateManager.getOutgoingCall().getNickname();
    }

    const std::string& Client::getNicknameInCallWith() const {
        return m_stateManager.getActiveCall().getNickname();
    }

    std::error_code Client::authorize(const std::string& nickname) {
        return m_authorizationService->authorize(nickname);
    }

    std::error_code Client::logout() {
        return m_authorizationService->logout();
    }

    std::error_code Client::startOutgoingCall(const std::string& userNickname) {
        // Проверяем, есть ли уже входящий звонок от этого пользователя
        auto& incomingCalls = m_stateManager.getIncomingCalls();
        bool acceptExistingCall = false;
        for (auto& [nickname, incomingCall] : incomingCalls) {
            if (nickname == userNickname) {
                acceptExistingCall = true;
                break;
            }
        }

        if (acceptExistingCall) {
            // Если есть входящий звонок, принимаем его
            return acceptCall(userNickname);
        }
        else {
            // Иначе начинаем исходящий звонок через сервис
            return m_callService->startOutgoingCall(userNickname);
        }
    }

    std::error_code Client::stopOutgoingCall() {
        return m_callService->stopOutgoingCall();
    }

    std::error_code Client::acceptCall(const std::string& userNickname) {
        return m_callService->acceptCall(userNickname);
    } 

    std::error_code Client::declineCall(const std::string& userNickname) {
        return m_callService->declineCall(userNickname);
    }

    std::error_code Client::endCall() {
        return m_callService->endCall();
    }

    std::error_code Client::startScreenSharing() {
        return m_mediaSharingService->startScreenSharing();
    }

    std::error_code Client::stopScreenSharing() {
        return m_mediaSharingService->stopScreenSharing();
    }

    std::error_code Client::sendScreen(const std::vector<unsigned char>& data) {
        return m_mediaSharingService->sendScreen(data);
    }

    std::error_code Client::startCameraSharing() {
        return m_mediaSharingService->startCameraSharing();
    }

    std::error_code Client::stopCameraSharing() {
        return m_mediaSharingService->stopCameraSharing();
    }

    std::error_code Client::sendCamera(const std::vector<unsigned char>& data) {
        return m_mediaSharingService->sendCamera(data);
    }

    void Client::onInputVoice(const unsigned char* data, int length) {
        if (!m_stateManager.isActiveCall() || m_stateManager.isConnectionDown()) return;

        const CryptoPP::SecByteBlock& callKey = m_stateManager.getActiveCall().getCallKey();

        auto cipherData = m_mediaEncryptionService.encryptMedia(data, length, callKey);
        if (!cipherData.empty()) {
            m_mediaController.send(std::move(cipherData), static_cast<uint32_t>(PacketType::VOICE));
        }
    }
}