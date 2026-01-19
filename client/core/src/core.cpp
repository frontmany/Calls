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
        reset();
        m_networkController.stop();
    }

    bool Client::start(const std::string& host,
        const std::string& port,
        std::shared_ptr<EventListener> eventListener)
    {
        m_eventListener = std::move(eventListener);
        m_packetProcessor = std::make_unique<PacketProcessor>(m_stateManager, m_keyManager, m_taskManager, m_networkController, m_audioEngine, m_eventListener);

        bool audioInitialized = m_audioEngine.init(std::bind(&Client::onInputVoice, this, _1, _2));

        bool networkInitialized = m_networkController.init(host, port,
            std::bind(&Client::onNetworkReceive, this, _1, _2, _3),
            std::bind(&Client::onConnectionDown, this),
            std::bind(&Client::onConnectionRestored, this)
        );

        m_keyManager.generateKeys();

        if (!m_networkController.isRunning())
            m_networkController.start();

        if (audioInitialized && networkInitialized) {
            return true;
        }
        else {
            LOG_ERROR("Calls client initialization failed - audio: {}, network: {}", audioInitialized, networkInitialized);
            return false;
        }
    }

    void Client::stop() {
        reset();
        m_networkController.stop();
    }

    void Client::reset() {
        m_stateManager.reset();
        m_keyManager.resetKeys();
        m_taskManager.cancelAllTasks();
        m_operationManager.clearAllOperations();

        if (m_audioEngine.isStream())
            m_audioEngine.stopStream();
    }

    void Client::createAndStartTask(
        const std::string& uid,
        const std::vector<unsigned char>& packet,
        PacketType packetType,
        std::function<void(std::optional<nlohmann::json>)> onCompletion,
        std::function<void(std::optional<nlohmann::json>)> onFailure)
    {
        m_taskManager.createTask(uid, 1500ms, 3,
            std::bind(&Client::sendPacket, this, packet, packetType),
            std::move(onCompletion),
            std::move(onFailure)
        );

        m_taskManager.startTask(uid);
    }

    void Client::onNetworkReceive(const unsigned char* data, int length, uint32_t type) {
        onReceive(data, length, static_cast<PacketType>(type));
    }

    void Client::onConnectionDown() {
        LOG_ERROR("Connection down");
        m_stateManager.setConnectionDown(true);
        m_operationManager.clearAllOperations();

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

        m_eventListener->onConnectionDown();
    }

    void Client::onConnectionRestored() {
        LOG_INFO("Connection restored");

        if (m_stateManager.isAuthorized()) {
            auto [uid, packet] = PacketFactory::getReconnectPacket(m_stateManager.getMyNickname(), m_stateManager.getMyToken());

            createAndStartTask(uid, packet, PacketType::RECONNECT,
                std::bind(&Client::onReconnectCompleted, this, _1),
                std::bind(&Client::onReconnectFailed, this, _1)
            );
        }
        else {
            m_stateManager.setConnectionDown(false);
            m_eventListener->onConnectionRestoredAuthorizationNeeded();
        }
    }

    void Client::sendPacket(const std::vector<unsigned char>& packet, PacketType packetType) {
        m_networkController.send(packet, static_cast<uint32_t>(packetType));
    }

    void Client::onReconnectCompleted(std::optional<nlohmann::json> completionContext) {
        auto& context = completionContext.value();

        m_stateManager.setConnectionDown(false);

        bool reconnected = context[RESULT].get<bool>();
        if (reconnected) {
            bool activeCall = context[IS_ACTIVE_CALL].get<bool>();

            m_eventListener->onConnectionRestored();

            if (!activeCall) {
                m_stateManager.setScreenSharing(false);
                m_stateManager.setCameraSharing(false);
                m_stateManager.setViewingRemoteScreen(false);
                m_stateManager.setViewingRemoteCamera(false);
                m_stateManager.clearCallState();
                m_audioEngine.stopStream();

                m_eventListener->onCallEndedByRemote({});
            }
        }
        else {
            reset();
            m_eventListener->onConnectionRestoredAuthorizationNeeded();
        }
    }

    void Client::onReconnectFailed(std::optional<nlohmann::json> failureContext) {
        LOG_ERROR("Reconnect task failed");
    }

    void Client::onAuthorizeCompleted(const std::string& nickname, std::optional<nlohmann::json> completionContext) {
        m_operationManager.removeOperation(UserOperationType::AUTHORIZE, nickname);

        if (completionContext.has_value()) {
            auto& context = completionContext.value();

            bool successfullyAuthorized = context[RESULT].get<bool>();

            if (successfullyAuthorized) {
                std::string token = context[TOKEN];

                m_stateManager.setMyNickname(nickname);
                m_stateManager.setMyToken(token);
                m_stateManager.setAuthorized(true);
                m_eventListener->onAuthorizationResult({});
            }
            else {
                m_eventListener->onAuthorizationResult(ErrorCode::taken_nickname);
            }
        }
    }

    void Client::onAuthorizeFailed(const std::string& nickname, std::optional<nlohmann::json> failureContext) {
        m_operationManager.removeOperation(UserOperationType::AUTHORIZE, nickname);

        if (!m_stateManager.isConnectionDown()) {
            LOG_ERROR("Authorization task failed");
            m_eventListener->onAuthorizationResult(ErrorCode::network_error);
        }
    }

    void Client::onLogoutCompleted(std::optional<nlohmann::json> completionContext) {
        m_operationManager.removeOperation(UserOperationType::LOGOUT, m_stateManager.getMyNickname());
        reset();
        m_eventListener->onLogoutCompleted();
    }

    void Client::onLogoutFailed(std::optional<nlohmann::json> failureContext) {
        m_operationManager.removeOperation(UserOperationType::LOGOUT, m_stateManager.getMyNickname());
        LOG_ERROR("Logout task failed");

        reset();
        m_eventListener->onLogoutCompleted();
    }

    void Client::onRequestUserInfoCompleted(const std::string& userNickname, std::optional<nlohmann::json> completionContext) {
        if (completionContext.has_value()) {
            auto& context = completionContext.value();

            bool userInfoFound = context[RESULT].get<bool>();

            if (userInfoFound) {
                const std::string& userPublicKeyString = context[PUBLIC_KEY];
                auto userPublicKey = crypto::deserializePublicKey(userPublicKeyString);

                CryptoPP::SecByteBlock callKey;
                crypto::generateAESKey(callKey);

                auto [uid, packet] = PacketFactory::getStartOutgoingCallPacket(m_stateManager.getMyNickname(), userNickname, m_keyManager.getMyPublicKey(), userPublicKey, callKey);

                createAndStartTask(uid, packet, PacketType::CALLING_BEGIN,
                    std::bind(&Client::onStartOutgoingCallCompleted, this, userNickname, _1),
                    std::bind(&Client::onStartOutgoingCallFailed, this, userNickname, _1)
                );
            }
            else {
                m_operationManager.removeOperation(UserOperationType::START_OUTGOING_CALL, userNickname);
                m_eventListener->onStartOutgoingCallResult(ErrorCode::unexisting_user);
            }
        }
    }

    void Client::onRequestUserInfoFailed(const std::string& userNickname, std::optional<nlohmann::json> failureContext) {
        m_operationManager.removeOperation(UserOperationType::START_OUTGOING_CALL, userNickname);
        LOG_ERROR("Request user info task failed");
        m_eventListener->onStartOutgoingCallResult(ErrorCode::network_error);
    }

    void Client::onStartOutgoingCallCompleted(const std::string& userNickname, std::optional<nlohmann::json> completionContext) {
        m_operationManager.removeOperation(UserOperationType::START_OUTGOING_CALL, userNickname);

        m_stateManager.setOutgoingCall(userNickname, 32s, std::bind(&Client::onOutgoingCallTimeout, this));

        m_eventListener->onStartOutgoingCallResult({});
    }

    void Client::onStartOutgoingCallFailed(const std::string& userNickname, std::optional<nlohmann::json> failureContext) {
        m_operationManager.removeOperation(UserOperationType::START_OUTGOING_CALL, userNickname);
        LOG_ERROR("Start outgoing call task failed");
        m_eventListener->onStartOutgoingCallResult(ErrorCode::network_error);
    }

    void Client::onOutgoingCallTimeout() {
        m_stateManager.clearCallState();
        m_eventListener->onOutgoingCallTimeout({});
    }

    void Client::onStopOutgoingCallCompleted(const std::string& nickname, std::optional<nlohmann::json> completionContext) {
        m_operationManager.removeOperation(UserOperationType::STOP_OUTGOING_CALL, nickname);
        m_eventListener->onStopOutgoingCallResult({});
    }

    void Client::onStopOutgoingCallFailed(const std::string& nickname, std::optional<nlohmann::json> failureContext) {
        m_operationManager.removeOperation(UserOperationType::STOP_OUTGOING_CALL, nickname);
        LOG_ERROR("Stop outgoing call task failed");
        m_eventListener->onStopOutgoingCallResult(ErrorCode::network_error);
    }

    void Client::onDeclineIncomingCallCompleted(const std::string& nickname, std::optional<nlohmann::json> completionContext) {
        m_stateManager.removeIncomingCall(nickname);
    }

    void Client::onDeclineIncomingCallFailed(std::optional<nlohmann::json> failureContext) {
        LOG_ERROR("Decline incoming call task failed (as part of accept call operation)");
    }

    void Client::onAcceptCallStopOutgoingCompleted(const std::string& userNickname, std::optional<nlohmann::json> completionContext) {
        auto& incomingCalls = m_stateManager.getIncomingCalls();
        auto& incomingCall = incomingCalls.at(userNickname);
        auto [uid, packet] = PacketFactory::getAcceptCallPacket(m_stateManager.getMyNickname(), userNickname, m_keyManager.getMyPublicKey(), incomingCall.getPublicKey(), incomingCall.getCallKey());

        createAndStartTask(uid, packet, PacketType::CALL_ACCEPT,
            std::bind(&Client::onAcceptCallCompleted, this, userNickname, _1),
            std::bind(&Client::onAcceptCallFailed, this, userNickname, _1)
        );
    }

    void Client::onAcceptCallStopOutgoingFailed(const std::string& userNickname, std::optional<nlohmann::json> failureContext) {
        m_operationManager.removeOperation(UserOperationType::ACCEPT_CALL, userNickname);
        LOG_ERROR("Stop outgoing call task failed (as part of accept call operation)");
        m_eventListener->onAcceptCallResult(ErrorCode::network_error, userNickname);
    }

    void Client::onAcceptCallEndActiveCompleted(const std::string& userNickname, std::optional<nlohmann::json> completionContext) {
        m_stateManager.setScreenSharing(false);
        m_stateManager.setCameraSharing(false);
        m_stateManager.setViewingRemoteScreen(false);
        m_stateManager.setViewingRemoteCamera(false);
        m_stateManager.clearCallState();
        m_audioEngine.stopStream();

        auto& incomingCalls = m_stateManager.getIncomingCalls();
        auto& incomingCall = incomingCalls.at(userNickname);
        auto [uid, packet] = PacketFactory::getAcceptCallPacket(m_stateManager.getMyNickname(), userNickname, m_keyManager.getMyPublicKey(), incomingCall.getPublicKey(), incomingCall.getCallKey());

        createAndStartTask(uid, packet, PacketType::CALL_ACCEPT,
            std::bind(&Client::onAcceptCallCompleted, this, userNickname, _1),
            std::bind(&Client::onAcceptCallFailedAfterEndCall, this, userNickname, _1)
        );
    }

    void Client::onAcceptCallEndActiveFailed(const std::string& userNickname, std::optional<nlohmann::json> failureContext) {
        m_operationManager.removeOperation(UserOperationType::ACCEPT_CALL, userNickname);
        LOG_ERROR("End active call task failed (as part of accept new call operation)");
        m_eventListener->onAcceptCallResult(ErrorCode::network_error, userNickname);
    }

    void Client::onAcceptCallCompleted(const std::string& userNickname, std::optional<nlohmann::json> completionContext) {
        m_operationManager.removeOperation(UserOperationType::ACCEPT_CALL, userNickname);

        auto& incomingCalls = m_stateManager.getIncomingCalls();
        auto& incomingCall = incomingCalls.at(userNickname);
        m_stateManager.setActiveCall(incomingCall.getNickname(), incomingCall.getPublicKey(), incomingCall.getCallKey());
        m_stateManager.removeIncomingCall(userNickname);

        m_audioEngine.startStream();
        m_eventListener->onAcceptCallResult({}, userNickname);
    }

    void Client::onAcceptCallFailed(const std::string& userNickname, std::optional<nlohmann::json> failureContext) {
        m_operationManager.removeOperation(UserOperationType::ACCEPT_CALL, userNickname);
        LOG_ERROR("Accept call task failed");
        m_eventListener->onAcceptCallResult(ErrorCode::network_error, userNickname);
    }

    void Client::onAcceptCallFailedAfterEndCall(const std::string& userNickname, std::optional<nlohmann::json> failureContext) {
        m_operationManager.removeOperation(UserOperationType::ACCEPT_CALL, userNickname);
        LOG_ERROR("Accept call task failed");
        m_eventListener->onEndCallResult({});
        m_eventListener->onAcceptCallResult(ErrorCode::network_error, userNickname);
    }

    void Client::onDeclineCallCompleted(const std::string& userNickname, std::optional<nlohmann::json> completionContext) {
        m_operationManager.removeOperation(UserOperationType::DECLINE_CALL, userNickname);

        m_stateManager.removeIncomingCall(userNickname);

        m_eventListener->onDeclineCallResult({}, userNickname);
    }

    void Client::onDeclineCallFailed(const std::string& userNickname, std::optional<nlohmann::json> failureContext) {
        m_operationManager.removeOperation(UserOperationType::DECLINE_CALL, userNickname);
        LOG_ERROR("Decline incoming call task failed");
        m_eventListener->onDeclineCallResult(make_error_code(ErrorCode::network_error), userNickname);
    }

    void Client::onEndCallCompleted(const std::string& nickname, std::optional<nlohmann::json> completionContext) {
        m_operationManager.removeOperation(UserOperationType::END_CALL, nickname);

        m_stateManager.setScreenSharing(false);
        m_stateManager.setCameraSharing(false);
        m_stateManager.setViewingRemoteScreen(false);
        m_stateManager.setViewingRemoteCamera(false);
        m_stateManager.clearCallState();
        m_audioEngine.stopStream();

        m_eventListener->onEndCallResult({});
    }

    void Client::onEndCallFailed(const std::string& nickname, std::optional<nlohmann::json> failureContext) {
        m_operationManager.removeOperation(UserOperationType::END_CALL, nickname);
        LOG_ERROR("Stop outgoing call task failed");
        m_eventListener->onEndCallResult(ErrorCode::network_error);
    }

    void Client::onStartScreenSharingCompleted(std::optional<nlohmann::json> completionContext) {
        const std::string& friendNickname = m_stateManager.getActiveCall().getNickname();
        m_operationManager.removeOperation(UserOperationType::START_SCREEN_SHARING, friendNickname);
        m_stateManager.setScreenSharing(true);
        m_eventListener->onStartScreenSharingResult({});
    }

    void Client::onStartScreenSharingFailed(std::optional<nlohmann::json> failureContext) {
        const std::string& friendNickname = m_stateManager.getActiveCall().getNickname();
        m_operationManager.removeOperation(UserOperationType::START_SCREEN_SHARING, friendNickname);
        LOG_ERROR("Start screen sharing task failed");
        m_eventListener->onStartScreenSharingResult(ErrorCode::network_error);
    }

    void Client::onStopScreenSharingCompleted(std::optional<nlohmann::json> completionContext) {
        const std::string& friendNickname = m_stateManager.getActiveCall().getNickname();
        m_operationManager.removeOperation(UserOperationType::STOP_SCREEN_SHARING, friendNickname);
        m_stateManager.setScreenSharing(false);
        m_eventListener->onStopScreenSharingResult({});
    }

    void Client::onStopScreenSharingFailed(std::optional<nlohmann::json> failureContext) {
        const std::string& friendNickname = m_stateManager.getActiveCall().getNickname();
        m_operationManager.removeOperation(UserOperationType::STOP_SCREEN_SHARING, friendNickname);
        LOG_ERROR("Stop screen sharing task failed");
        m_eventListener->onStopScreenSharingResult(ErrorCode::network_error);
    }

    void Client::onStartCameraSharingCompleted(std::optional<nlohmann::json> completionContext) {
        const std::string& friendNickname = m_stateManager.getActiveCall().getNickname();
        m_operationManager.removeOperation(UserOperationType::START_CAMERA_SHARING, friendNickname);
        m_stateManager.setCameraSharing(true);
        m_eventListener->onStartCameraSharingResult({});
    }

    void Client::onStartCameraSharingFailed(std::optional<nlohmann::json> failureContext) {
        const std::string& friendNickname = m_stateManager.getActiveCall().getNickname();
        m_operationManager.removeOperation(UserOperationType::START_CAMERA_SHARING, friendNickname);
        LOG_ERROR("Start camera sharing task failed");
        m_eventListener->onStartCameraSharingResult(ErrorCode::network_error);
    }

    void Client::onStopCameraSharingCompleted(std::optional<nlohmann::json> completionContext) {
        const std::string& friendNickname = m_stateManager.getActiveCall().getNickname();
        m_operationManager.removeOperation(UserOperationType::STOP_CAMERA_SHARING, friendNickname);
        m_stateManager.setCameraSharing(false);
        m_eventListener->onStopCameraSharingResult({});
    }

    void Client::onStopCameraSharingFailed(std::optional<nlohmann::json> failureContext) {
        const std::string& friendNickname = m_stateManager.getActiveCall().getNickname();
        m_operationManager.removeOperation(UserOperationType::STOP_CAMERA_SHARING, friendNickname);
        LOG_ERROR("Stop camera sharing task failed");
        m_eventListener->onStopCameraSharingResult(ErrorCode::network_error);
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
        if (m_stateManager.isConnectionDown()) return make_error_code(ErrorCode::connection_down);
        if (m_stateManager.isAuthorized()) return make_error_code(ErrorCode::already_authorized);
        if (m_operationManager.isOperation(UserOperationType::AUTHORIZE, nickname)) return make_error_code(ErrorCode::operation_in_progress);

        if (!m_keyManager.isKeys()) {
            if (m_keyManager.isGeneratingKeys()) {
            }
            else {
                m_keyManager.generateKeys();
            }
        }

        m_keyManager.awaitKeysGeneration();

        m_operationManager.addOperation(UserOperationType::AUTHORIZE, nickname);

        auto [uid, packet] = PacketFactory::getAuthorizationPacket(nickname, m_keyManager.getMyPublicKey());

        createAndStartTask(uid, packet, PacketType::AUTHORIZATION,
            std::bind(&Client::onAuthorizeCompleted, this, nickname, _1),
            std::bind(&Client::onAuthorizeFailed, this, nickname, _1)
        );

        return {};
    }

    std::error_code Client::logout() {
        if (isConnectionDown()) return make_error_code(ErrorCode::connection_down);
        if (!m_stateManager.isAuthorized()) return make_error_code(ErrorCode::not_authorized);
        if (m_operationManager.isOperation(UserOperationType::LOGOUT, m_stateManager.getMyNickname())) return make_error_code(ErrorCode::operation_in_progress);

        m_operationManager.addOperation(UserOperationType::LOGOUT, m_stateManager.getMyNickname());

        auto [uid, packet] = PacketFactory::getLogoutPacket(m_stateManager.getMyNickname());

        createAndStartTask(uid, packet, PacketType::LOGOUT,
            std::bind(&Client::onLogoutCompleted, this, _1),
            std::bind(&Client::onLogoutFailed, this, _1)
        );

        return {};
    }

    std::error_code Client::startOutgoingCall(const std::string& userNickname) {
        if (m_stateManager.isConnectionDown()) return make_error_code(ErrorCode::connection_down);
        if (!m_stateManager.isAuthorized()) return make_error_code(ErrorCode::not_authorized);
        if (m_stateManager.isActiveCall()) return make_error_code(ErrorCode::active_call_exists);
        if (m_operationManager.isOperation(UserOperationType::START_OUTGOING_CALL, userNickname)) return make_error_code(ErrorCode::operation_in_progress);

        auto& icomingCalls = m_stateManager.getIncomingCalls();

        bool acceptExistingCall = false;
        for (auto& [nickname, incomingCall] : icomingCalls) {
            if (nickname == userNickname) {
                acceptExistingCall = true;
            }
        }

        if (acceptExistingCall) {
            acceptCall(userNickname);
        }
        else {
            m_operationManager.addOperation(UserOperationType::START_OUTGOING_CALL, userNickname);

            auto [uid, packet] = PacketFactory::getRequestUserInfoPacket(m_stateManager.getMyNickname(), userNickname);

            createAndStartTask(uid, packet, PacketType::GET_USER_INFO,
                std::bind(&Client::onRequestUserInfoCompleted, this, userNickname, _1),
                std::bind(&Client::onRequestUserInfoFailed, this, userNickname, _1)
            );
        }

        return {};
    }

    std::error_code Client::stopOutgoingCall() {
        if (m_stateManager.isConnectionDown()) return make_error_code(ErrorCode::connection_down);
        if (!m_stateManager.isAuthorized()) return make_error_code(ErrorCode::not_authorized);
        if (!m_stateManager.isOutgoingCall()) return make_error_code(ErrorCode::no_outgoing_call);
        const std::string& nickname = m_stateManager.getOutgoingCall().getNickname();
        if (m_operationManager.isOperation(UserOperationType::STOP_OUTGOING_CALL, nickname)) return make_error_code(ErrorCode::operation_in_progress);

        m_operationManager.addOperation(UserOperationType::STOP_OUTGOING_CALL, nickname);

        auto [uid, packet] = PacketFactory::getStopOutgoingCallPacket(m_stateManager.getMyNickname(), nickname);

        createAndStartTask(uid, packet, PacketType::CALLING_END,
            std::bind(&Client::onStopOutgoingCallCompleted, this, nickname, _1),
            std::bind(&Client::onStopOutgoingCallFailed, this, nickname, _1)
        );

        return {};
    }

    std::error_code Client::acceptCall(const std::string& userNickname) {
        if (m_stateManager.isConnectionDown()) return make_error_code(ErrorCode::connection_down);
        if (!m_stateManager.isAuthorized()) return make_error_code(ErrorCode::not_authorized);
        if (!m_stateManager.isIncomingCalls()) return make_error_code(ErrorCode::no_incoming_call);
        if (m_operationManager.isOperation(UserOperationType::ACCEPT_CALL, userNickname)) return make_error_code(ErrorCode::operation_in_progress);

        auto& incomingCalls = m_stateManager.getIncomingCalls();
        if (!incomingCalls.contains(userNickname)) return make_error_code(ErrorCode::no_incoming_call);

        m_operationManager.addOperation(UserOperationType::ACCEPT_CALL, userNickname);

        for (auto& [nickname, incomingCallData] : incomingCalls) {
            if (nickname != userNickname) {
                auto [uid, packet] = PacketFactory::getDeclineCallPacket(m_stateManager.getMyNickname(), nickname);

                createAndStartTask(uid, packet, PacketType::CALL_DECLINE,
                    std::bind(&Client::onDeclineIncomingCallCompleted, this, nickname, _1),
                    std::bind(&Client::onDeclineIncomingCallFailed, this, _1)
                );
            }
        }

        if (m_stateManager.isOutgoingCall()) {
            auto [uid, packet] = PacketFactory::getStopOutgoingCallPacket(m_stateManager.getMyNickname(), m_stateManager.getOutgoingCall().getNickname());

            createAndStartTask(uid, packet, PacketType::CALLING_END,
                std::bind(&Client::onAcceptCallStopOutgoingCompleted, this, userNickname, _1),
                std::bind(&Client::onAcceptCallStopOutgoingFailed, this, userNickname, _1)
            );
        }
        else if (m_stateManager.isActiveCall()) {
            auto [uid, packet] = PacketFactory::getEndCallPacket(m_stateManager.getMyNickname(), m_stateManager.getActiveCall().getNickname());

            createAndStartTask(uid, packet, PacketType::CALL_END,
                std::bind(&Client::onAcceptCallEndActiveCompleted, this, userNickname, _1),
                std::bind(&Client::onAcceptCallEndActiveFailed, this, userNickname, _1)
            );
        }
        else {
            auto& incomingCalls = m_stateManager.getIncomingCalls();
            auto& incomingCall = incomingCalls.at(userNickname);
            auto [uid, packet] = PacketFactory::getAcceptCallPacket(m_stateManager.getMyNickname(), userNickname, m_keyManager.getMyPublicKey(), incomingCall.getPublicKey(), incomingCall.getCallKey());

            createAndStartTask(uid, packet, PacketType::CALL_ACCEPT,
                std::bind(&Client::onAcceptCallCompleted, this, userNickname, _1),
                std::bind(&Client::onAcceptCallFailed, this, userNickname, _1)
            );
        }

        return {};
    } 

    std::error_code Client::declineCall(const std::string& userNickname) {
        if (m_stateManager.isConnectionDown()) return make_error_code(ErrorCode::connection_down);
        if (!m_stateManager.isAuthorized()) return make_error_code(ErrorCode::not_authorized);
        if (!m_stateManager.isIncomingCalls()) return make_error_code(ErrorCode::no_incoming_call);
        if (m_operationManager.isOperation(UserOperationType::DECLINE_CALL, userNickname)) return make_error_code(ErrorCode::operation_in_progress);

        auto& incomingCalls = m_stateManager.getIncomingCalls();
        if (!incomingCalls.contains(userNickname)) return make_error_code(ErrorCode::no_incoming_call);

        m_operationManager.addOperation(UserOperationType::DECLINE_CALL, userNickname);

        auto [uid, packet] = PacketFactory::getDeclineCallPacket(m_stateManager.getMyNickname(), userNickname);

        createAndStartTask(uid, packet, PacketType::CALL_DECLINE,
            std::bind(&Client::onDeclineCallCompleted, this, userNickname, _1),
            std::bind(&Client::onDeclineCallFailed, this, userNickname, _1)
        );

        return {};
    }

    std::error_code Client::endCall() {
        if (m_stateManager.isConnectionDown()) return make_error_code(ErrorCode::connection_down);
        if (!m_stateManager.isAuthorized()) return make_error_code(ErrorCode::not_authorized);
        if (!m_stateManager.isActiveCall()) return make_error_code(ErrorCode::no_active_call);
        const std::string& nickname = m_stateManager.getActiveCall().getNickname();
        if (m_operationManager.isOperation(UserOperationType::END_CALL, nickname)) return make_error_code(ErrorCode::operation_in_progress);

        m_operationManager.addOperation(UserOperationType::END_CALL, nickname);

        auto [uid, packet] = PacketFactory::getEndCallPacket(m_stateManager.getMyNickname(), nickname);

        createAndStartTask(uid, packet, PacketType::CALL_END,
            std::bind(&Client::onEndCallCompleted, this, nickname, _1),
            std::bind(&Client::onEndCallFailed, this, nickname, _1)
        );

        return {};
    }

    std::error_code Client::startScreenSharing() {
        if (m_stateManager.isConnectionDown()) return make_error_code(ErrorCode::connection_down);
        if (!m_stateManager.isAuthorized()) return make_error_code(ErrorCode::not_authorized);
        if (!m_stateManager.isActiveCall()) return make_error_code(ErrorCode::no_active_call);
        if (m_stateManager.isScreenSharing()) return make_error_code(ErrorCode::screen_sharing_already_active);
        if (m_stateManager.isViewingRemoteScreen()) return make_error_code(ErrorCode::viewing_remote_screen);
        const std::string& friendNickname = m_stateManager.getActiveCall().getNickname();
        if (m_operationManager.isOperation(UserOperationType::START_SCREEN_SHARING, friendNickname)) return make_error_code(ErrorCode::operation_in_progress);

        m_operationManager.addOperation(UserOperationType::START_SCREEN_SHARING, friendNickname);
        auto [uid, packet] = PacketFactory::getStartScreenSharingPacket(m_stateManager.getMyNickname(), friendNickname);

        createAndStartTask(uid, packet, PacketType::SCREEN_SHARING_BEGIN,
            std::bind(&Client::onStartScreenSharingCompleted, this, _1),
            std::bind(&Client::onStartScreenSharingFailed, this, _1)
        );

        return {};
    }

    std::error_code Client::stopScreenSharing() {
        if (m_stateManager.isConnectionDown()) return make_error_code(ErrorCode::connection_down);
        if (!m_stateManager.isAuthorized()) return make_error_code(ErrorCode::not_authorized);
        if (!m_stateManager.isActiveCall()) return make_error_code(ErrorCode::no_active_call);
        if (!m_stateManager.isScreenSharing()) return make_error_code(ErrorCode::screen_sharing_not_active);
        const std::string& friendNickname = m_stateManager.getActiveCall().getNickname();
        if (m_operationManager.isOperation(UserOperationType::STOP_SCREEN_SHARING, friendNickname)) return make_error_code(ErrorCode::operation_in_progress);

        m_operationManager.addOperation(UserOperationType::STOP_SCREEN_SHARING, friendNickname);
        auto [uid, packet] = PacketFactory::getStopScreenSharingPacket(m_stateManager.getMyNickname(), friendNickname);

        createAndStartTask(uid, packet, PacketType::SCREEN_SHARING_END,
            std::bind(&Client::onStopScreenSharingCompleted, this, _1),
            std::bind(&Client::onStopScreenSharingFailed, this, _1)
        );

        return {};
    }

    std::error_code Client::sendScreen(const std::vector<unsigned char>& data) {
        if (m_stateManager.isConnectionDown()) return make_error_code(ErrorCode::connection_down);
        if (!m_stateManager.isAuthorized()) return make_error_code(ErrorCode::not_authorized);
        if (!m_stateManager.isActiveCall()) return make_error_code(ErrorCode::no_active_call);
        if (!m_stateManager.isScreenSharing()) return make_error_code(ErrorCode::screen_sharing_not_active);

        const CryptoPP::SecByteBlock& callKey = m_stateManager.getActiveCall().getCallKey();

        try {
            size_t cipherDataLength = data.size() + CryptoPP::AES::BLOCKSIZE;
            std::vector<CryptoPP::byte> cipherData(cipherDataLength);

            crypto::AESEncrypt(callKey,
                reinterpret_cast<const CryptoPP::byte*>(data.data()),
                data.size(),
                cipherData.data(),
                cipherDataLength);

            m_networkController.send(std::move(cipherData), static_cast<uint32_t>(PacketType::SCREEN));
            return {};
        }
        catch (const std::exception& e) {
            LOG_ERROR("Screen sending error");
            return make_error_code(ErrorCode::encryption_error);
        }
    }

    std::error_code Client::startCameraSharing() {
        if (m_stateManager.isConnectionDown()) return make_error_code(ErrorCode::connection_down);
        if (!m_stateManager.isAuthorized()) return make_error_code(ErrorCode::not_authorized);
        if (!m_stateManager.isActiveCall()) return make_error_code(ErrorCode::no_active_call);
        if (m_stateManager.isCameraSharing()) return make_error_code(ErrorCode::camera_sharing_already_active);
        const std::string& friendNickname = m_stateManager.getActiveCall().getNickname();
        if (m_operationManager.isOperation(UserOperationType::START_CAMERA_SHARING, friendNickname)) return make_error_code(ErrorCode::operation_in_progress);

        m_operationManager.addOperation(UserOperationType::START_CAMERA_SHARING, friendNickname);
        auto [uid, packet] = PacketFactory::getStartCameraSharingPacket(m_stateManager.getMyNickname(), friendNickname);

        createAndStartTask(uid, packet, PacketType::CAMERA_SHARING_BEGIN,
            std::bind(&Client::onStartCameraSharingCompleted, this, _1),
            std::bind(&Client::onStartCameraSharingFailed, this, _1)
        );

        return {};
    }

    std::error_code Client::stopCameraSharing() {
        if (m_stateManager.isConnectionDown()) return make_error_code(ErrorCode::connection_down);
        if (!m_stateManager.isAuthorized()) return make_error_code(ErrorCode::not_authorized);
        if (!m_stateManager.isActiveCall()) return make_error_code(ErrorCode::no_active_call);
        if (!m_stateManager.isCameraSharing()) return make_error_code(ErrorCode::camera_sharing_not_active);
        const std::string& friendNickname = m_stateManager.getActiveCall().getNickname();
        if (m_operationManager.isOperation(UserOperationType::STOP_CAMERA_SHARING, friendNickname)) return make_error_code(ErrorCode::operation_in_progress);

        m_operationManager.addOperation(UserOperationType::STOP_CAMERA_SHARING, friendNickname);
        auto [uid, packet] = PacketFactory::getStopCameraSharingPacket(m_stateManager.getMyNickname(), friendNickname);

        createAndStartTask(uid, packet, PacketType::CAMERA_SHARING_END,
            std::bind(&Client::onStopCameraSharingCompleted, this, _1),
            std::bind(&Client::onStopCameraSharingFailed, this, _1)
        );

        return {};
    }

    std::error_code Client::sendCamera(const std::vector<unsigned char>& data) {
        if (m_stateManager.isConnectionDown()) return make_error_code(ErrorCode::connection_down);
        if (!m_stateManager.isAuthorized()) return make_error_code(ErrorCode::not_authorized);
        if (!m_stateManager.isActiveCall()) return make_error_code(ErrorCode::no_active_call);
        if (!m_stateManager.isCameraSharing()) return make_error_code(ErrorCode::camera_sharing_not_active);

        const CryptoPP::SecByteBlock& callKey = m_stateManager.getActiveCall().getCallKey();

        try {
            size_t cipherDataLength = data.size() + CryptoPP::AES::BLOCKSIZE;
            std::vector<CryptoPP::byte> cipherData(cipherDataLength);

            crypto::AESEncrypt(callKey,
                reinterpret_cast<const CryptoPP::byte*>(data.data()),
                data.size(),
                cipherData.data(),
                cipherDataLength);

            m_networkController.send(std::move(cipherData), static_cast<uint32_t>(PacketType::CAMERA));
            return {};
        }
        catch (const std::exception& e) {
            LOG_ERROR("Camera sending error");
            return make_error_code(ErrorCode::encryption_error);
        }
    }

    void Client::onInputVoice(const unsigned char* data, int length) {
        if (!m_stateManager.isActiveCall() || m_stateManager.isConnectionDown()) return;

        const CryptoPP::SecByteBlock& callKey = m_stateManager.getActiveCall().getCallKey();

        size_t cipherDataLength = static_cast<size_t>(length) + CryptoPP::AES::BLOCKSIZE;
        std::vector<CryptoPP::byte> cipherData(cipherDataLength);
        crypto::AESEncrypt(callKey, data, length, cipherData.data(), cipherDataLength);
        
        m_networkController.send(std::move(cipherData), static_cast<uint32_t>(PacketType::VOICE));
    }
}