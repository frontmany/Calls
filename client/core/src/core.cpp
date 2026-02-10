#include "core.h"
#include "jsonType.h"
#include "packetFactory.h"
#include "utilities/crypto.h"

#include "keyManager.h"
#include "utilities/errorCode.h"
#include "network/tcp/client.h"
#include "network/udp/client.h"
#include "packetType.h"
#include "eventListener.h"
#include "packetProcessingService.h"
#include "authorizationService.h"
#include "callService.h"


#include "utilities/logger.h"
#include "utilities/crashCatchInitializer.h"
#include "constants/errorCode.h"

#include <filesystem>

using namespace core::utilities;
using namespace core::constant;
using namespace std::chrono_literals;

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

    Client::~Client() {
        stop();
    }

    bool Client::start(
        const std::string& tcpHost,
        const std::string& udpHost,
        const std::string& tcpPort,
        const std::string& udpPort,
        std::shared_ptr<EventListener> eventListener)
    {
        m_networkController = std::make_unique<network::NetworkController>();
        bool tcpConnected = m_networkController->connectTCP(tcpHost, tcpPort);
        bool udpStarted = m_networkController->runUDP(udpHost, udpPort);

        if (!tcpConnected) {
            LOG_ERROR("TCP client connect error");
            stop();

            return false;
        }
        if (!udpStarted) {
            LOG_ERROR("UDP client starting error");
            stop();

            return false;
        }

        auto mediaProcessingService = std::make_shared<media::MediaProcessingService>();
        bool audioProcessingInitialized = mediaProcessingService->initializeAudioProcessing();
        bool videoProcessingInitialized = mediaProcessingService->initializeVideoProcessing();

        if (!audioProcessingInitialized) {
            LOG_ERROR("audio processing service initialization error");
            stop();

            return false;
        }
        if (!videoProcessingInitialized) {
            LOG_ERROR("video processing service initialization error");
            stop();

            return false;
        }

        m_audioEngine = std::make_shared<media::AudioEngine>();
        bool audioInitialized = m_audioEngine->initialize();

        if (!audioInitialized) {
            LOG_ERROR("AudioEngine nitialization error");
            stop();

            return false;
        }

        m_stateManager = std::make_shared<logic::ClientStateManager>();

        auto keyManager = std::make_shared<logic::KeyManager>();
        keyManager->generateKeys();

        m_authorizationService = std::make_unique<logic::AuthorizationService>(m_stateManager, keyManager,
            [this]() {return m_networkController->getUDPLocalPort(); },
            [this](const std::vector<unsigned char>& packet, core::constant::PacketType type) {m_networkController->sendTCP(packet, type); }
        );
        
        m_callService = std::make_unique<logic::CallService>(m_stateManager, keyManager,
            [this](const std::vector<unsigned char>& packet, constant::PacketType type) {m_networkController->sendTCP(packet, type); }
        );

        m_mediaService = std::make_unique<logic::MediaService>(m_stateManager, m_audioEngine, mediaProcessingService, eventListener,
            [this](const std::vector<unsigned char>& packet, constant::PacketType type) {m_networkController->sendTCP(packet, type); },
            [this](const std::vector<unsigned char>& data, constant::PacketType type) {m_networkController->sendUDP(data, type); }
        );

        m_reconnectionService = std::make_unique<logic::ReconnectionService>(m_stateManager, keyManager,
            [this]() {return m_networkController->getUDPLocalPort(); },
            [this](const std::vector<unsigned char>& packet, core::constant::PacketType type) {m_networkController->sendTCP(packet, type); },
            [this]() {return m_networkController->tryReconnectTCP(5); }
        );

        m_packetHandleController = std::make_unique<logic::PacketHandleController>(m_stateManager, keyManager, m_audioEngine, mediaProcessingService, eventListener,
            [this](const std::vector<unsigned char>& packet, core::constant::PacketType type) {m_networkController->sendTCP(packet, type); }
        );

        return true;
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
        m_stateManager->reset();
        m_keyManager.resetKeys();
        m_pendingRequests.failAll(std::nullopt);
        m_operationManager.clearAllOperations();

        if (m_audioEngine->isStream())
            m_audioEngine->stopStream();
    }

    std::vector<std::string> Client::getCallers() const {
        auto& incomingCalls = m_stateManager->getIncomingCalls();

        std::vector<std::string> callersNicknames;
        for (auto& [nickname, incomingCall] : incomingCalls) {
            callersNicknames.push_back(nickname);
        }

        return callersNicknames;
    }

    void Client::muteMicrophone(bool isMute) {
        m_audioEngine->muteMicrophone(isMute);
    }

    void Client::muteSpeaker(bool isMute) {
        m_audioEngine->muteSpeaker(isMute);
    }

    bool Client::isScreenSharing() const {
        return m_stateManager->isScreenSharing();
    }

    bool Client::isViewingRemoteScreen() const {
        return m_stateManager->isViewingRemoteScreen();
    }

    bool Client::isCameraSharing() const {
        return m_stateManager->isCameraSharing();
    }

    bool Client::isViewingRemoteCamera() const {
        return m_stateManager->isViewingRemoteCamera();
    }

    bool Client::isMicrophoneMuted() const {
        return m_audioEngine->isMicrophoneMuted();
    }

    bool Client::isSpeakerMuted() const {
        return m_audioEngine->isSpeakerMuted();
    }

    void Client::refreshAudioDevices() {
        m_audioEngine->refreshAudioDevices();
    }

    int Client::getInputVolume() const {
        return m_audioEngine->getInputVolume();
    }

    int Client::getOutputVolume() const {
        return m_audioEngine->getOutputVolume();
    }

    void Client::setInputVolume(int volume) {
        m_audioEngine->setInputVolume(volume);
    }

    void Client::setOutputVolume(int volume) {
        m_audioEngine->setOutputVolume(volume);
    }

    bool Client::setInputDevice(int deviceIndex) {
        return m_audioEngine->setInputDevice(deviceIndex);
    }

    bool Client::setOutputDevice(int deviceIndex) {
        return m_audioEngine->setOutputDevice(deviceIndex);
    }

    int Client::getCurrentInputDevice() const {
        return m_audioEngine->getCurrentInputDevice();
    }

    int Client::getCurrentOutputDevice() const {
        return m_audioEngine->getCurrentOutputDevice();
    }

    bool Client::isAuthorized() const {
        return m_stateManager->isAuthorized();
    }

    bool Client::isOutgoingCall() const {
        return m_stateManager->isOutgoingCall();
    }

    bool Client::isActiveCall() const {
        return m_stateManager->isActiveCall();
    }

    bool Client::isConnectionDown() const {
        return m_stateManager->isConnectionDown();
    }

    int Client::getIncomingCallsCount() const {
        return m_stateManager->getIncomingCallsCount();
    }

    const std::string& Client::getMyNickname() const {
        return m_stateManager->getMyNickname();
    }

    const std::string& Client::getNicknameWhomCalling() const {
        return m_stateManager->getOutgoingCall().getNickname();
    }

    const std::string& Client::getNicknameInCallWith() const {
        return m_stateManager->getActiveCall().getNickname();
    }



    std::error_code Client::authorize(const std::string& nickname) {
        return m_authorizationService->authorize(nickname);
    }

    std::error_code Client::logout() {
        return m_authorizationService->logout();
    }

    std::error_code Client::startOutgoingCall(const std::string& userNickname) {
        m_callService->startOutgoingCall(userNickname);
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

    std::error_code Client::startScreenSharing(int screenIndex) {
        if (!m_stateManager->isActiveCall()) return make_error_code(ErrorCode::no_active_call);

        return m_mediaService->startScreenSharing(m_stateManager->getMyNickname(), m_stateManager->getActiveCall().getNickname(), screenIndex);
    }

    std::error_code Client::stopScreenSharing() {
        if (!m_stateManager->isActiveCall()) return make_error_code(ErrorCode::no_active_call);

        return m_mediaService->stopScreenSharing(m_stateManager->getMyNickname(), m_stateManager->getActiveCall().getNickname());
    }

    std::error_code Client::startCameraSharing(std::string deviceName) {
        if (!m_stateManager->isActiveCall()) return make_error_code(ErrorCode::no_active_call);

        return m_mediaService->startCameraSharing(m_stateManager->getMyNickname(), m_stateManager->getActiveCall().getNickname(), deviceName);
    }

    std::error_code Client::stopCameraSharing() {
        if (!m_stateManager->isActiveCall()) return make_error_code(ErrorCode::no_active_call);

        return m_mediaService->stopCameraSharing(m_stateManager->getMyNickname(), m_stateManager->getActiveCall().getNickname());
    }
}