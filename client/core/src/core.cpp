#include "core.h"
#include "utilities/logger.h"
#include "utilities/crashCatchInitializer.h"
#include "constants/errorCode.h"
#include "logic/packetFactory.h"
#include "constants/packetType.h"

#include <chrono>
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

        core::utilities::initializeCrashCatch((crashDir / "Core").string(), appVersion);
    }

    Core::~Core() {
        stop();
    }

    bool Core::start(
        const std::string& tcpHost,
        const std::string& udpHost,
        const std::string& tcpPort,
        const std::string& udpPort,
        std::shared_ptr<EventListener> eventListener)
    {
        m_stateManager = std::make_shared<logic::ClientStateManager>();
        m_stateManager->setConnectionDown(true);

        m_tcpHost = tcpHost;
        m_udpHost = udpHost;
        m_tcpPort = tcpPort;
        m_udpPort = udpPort;

        m_networkController = std::make_unique<network::NetworkController>(
            [this](const unsigned char* data, int size, constant::PacketType type) {
                if (m_packetHandleController)
                    m_packetHandleController->processPacket(data, size, type);
            },
            [this, eventListener]() {
                if (m_stateManager)
                    m_stateManager->setConnectionDown(true);
                if (m_connectionEstablishService)
                    m_connectionEstablishService->startConnectionAttempts();
                if (eventListener)
                    eventListener->onConnectionDown();
            }
        );

        if (!initializeServices(eventListener)) {
            stop();
            if (eventListener)
                eventListener->onConnectionDown();

            return false;
        }

        auto attemptEstablishConnection = [this]() {
            return m_networkController->establishConnection(m_tcpHost, m_tcpPort, m_udpHost, m_udpPort);
        };

        auto onConnectionEstablished = [this, eventListener]() {
            m_stateManager->setConnectionDown(false);
            if (m_stateManager->isAuthorized()) {
                sendReconnectPacket();
            }
            else if (eventListener) {
                eventListener->onConnectionEstablishedAuthorizationNeeded();
            }
        };

        m_connectionEstablishService = std::make_unique<logic::ConnectionEstablishService>(
            m_stateManager,
            std::move(attemptEstablishConnection),
            std::move(onConnectionEstablished),
            std::chrono::milliseconds(2000)
        );

        m_connectionEstablishService->startConnectionAttempts();
        
        return true;
    }

    bool Core::initializeServices(std::shared_ptr<EventListener> eventListener)
    {
        auto keyManager = std::make_shared<logic::KeyManager>();
        keyManager->generateKeys();

        auto mediaProcessingService = std::make_shared<media::MediaProcessingService>();
        bool audioProcessingInitialized = mediaProcessingService->initializeAudioProcessing();
        bool screenVideoInitialized = mediaProcessingService->initializeVideoProcessing(media::MediaType::Screen, 2400000);
        bool cameraVideoInitialized = mediaProcessingService->initializeVideoProcessing(media::MediaType::Camera, 900000);

        if (!audioProcessingInitialized) {
            LOG_ERROR("audio processing service initialization error");
            return false;
        }
        if (!screenVideoInitialized || !cameraVideoInitialized) {
            LOG_ERROR("video processing service initialization error");
            return false;
        }

        m_audioEngine = std::make_shared<media::AudioEngine>();
        bool audioInitialized = m_audioEngine->initialize();

        if (!audioInitialized) {
            LOG_ERROR("AudioEngine initialization error");
            return false;
        }

        auto sendTcp = [this](const std::vector<unsigned char>& packet, constant::PacketType type) -> std::error_code {
            return m_networkController->sendTCP(packet, type)
                ? std::error_code{}
                : core::constant::make_error_code(core::constant::ErrorCode::network_error);
        };
        auto sendUdp = [this](const std::vector<unsigned char>& data, constant::PacketType type) -> std::error_code {
            return m_networkController->sendUDP(data, type)
                ? std::error_code{}
                : core::constant::make_error_code(core::constant::ErrorCode::network_error);
        };

        m_authorizationService = std::make_unique<logic::AuthorizationService>(m_stateManager, keyManager,
            [this]() { return m_networkController->getUDPLocalPort(); },
            [sendTcp](const std::vector<unsigned char>& p, constant::PacketType t) { return sendTcp(p, t); }
        );
        
        m_callService = std::make_unique<logic::CallService>(m_stateManager, keyManager,
            [sendTcp](const std::vector<unsigned char>& p, constant::PacketType t) { return sendTcp(p, t); });

        m_mediaService = std::make_unique<logic::MediaService>(m_stateManager, m_audioEngine, mediaProcessingService, eventListener,
            [sendTcp](const std::vector<unsigned char>& p, constant::PacketType t) { return sendTcp(p, t); },
            [sendUdp](const std::vector<unsigned char>& d, constant::PacketType t) { return sendUdp(d, t); });

        m_packetHandleController = std::make_unique<logic::PacketHandleController>(m_stateManager, keyManager, m_audioEngine, mediaProcessingService, eventListener,
            [sendTcp](const std::vector<unsigned char>& p, constant::PacketType t) { return sendTcp(p, t); },
            [this]() { if (m_mediaService) m_mediaService->startAudioSharing(); },
            [this]() { if (m_mediaService) m_mediaService->stopAudioSharing(); });

        return true;
    }

    void Core::sendReconnectPacket() {
        if (!m_stateManager->isAuthorized() || !m_networkController)
            return;
        auto packet = logic::PacketFactory::getReconnectPacket(
            m_stateManager->getMyNickname(),
            m_stateManager->getMyToken(),
            m_networkController->getUDPLocalPort()
        );
        m_networkController->sendTCP(packet, PacketType::RECONNECT);
    }

    void Core::stop() {
        if (m_connectionEstablishService) {
            m_connectionEstablishService->stopConnectionAttempts();
        }

        if (m_audioEngine && m_audioEngine->isStream()) {
            m_audioEngine->stopAudioCapture();
        }

        m_packetHandleController.reset();
        m_connectionEstablishService.reset();
        m_mediaService.reset();
        m_callService.reset();
        m_authorizationService.reset();

        if (m_stateManager) {
            m_stateManager->reset();
        }

        m_networkController.reset();
        m_audioEngine.reset();
        m_stateManager.reset();
    }

    std::vector<std::string> Core::getCallers() const {
        if (!m_stateManager) return {};
        auto& incomingCalls = m_stateManager->getIncomingCalls();
        std::vector<std::string> callersNicknames;
        for (auto& [nickname, incomingCall] : incomingCalls) {
            callersNicknames.push_back(nickname);
        }
        return callersNicknames;
    }

    void Core::muteMicrophone(bool isMute) {
        if (m_audioEngine) m_audioEngine->muteMicrophone(isMute);
    }

    void Core::muteSpeaker(bool isMute) {
        if (m_audioEngine) m_audioEngine->muteSpeaker(isMute);
    }

    bool Core::isScreenSharing() const {
        return m_stateManager && m_stateManager->getMediaState(media::MediaType::Screen) == media::MediaState::Active;
    }

    bool Core::isViewingRemoteScreen() const {
        return m_stateManager ? m_stateManager->isViewingRemoteScreen() : false;
    }

    bool Core::isCameraSharing() const {
        return m_stateManager && m_stateManager->getMediaState(media::MediaType::Camera) == media::MediaState::Active;
    }

    bool Core::isViewingRemoteCamera() const {
        return m_stateManager ? m_stateManager->isViewingRemoteCamera() : false;
    }

    bool Core::isMicrophoneMuted() const {
        return m_audioEngine ? m_audioEngine->isMicrophoneMuted() : false;
    }

    bool Core::isSpeakerMuted() const {
        return m_audioEngine ? m_audioEngine->isSpeakerMuted() : false;
    }

    void Core::refreshAudioDevices() {
        if (m_audioEngine) m_audioEngine->refreshAudioDevices();
    }

    int Core::getInputVolume() const {
        return m_audioEngine ? m_audioEngine->getInputVolume() : 100;
    }

    int Core::getOutputVolume() const {
        return m_audioEngine ? m_audioEngine->getOutputVolume() : 100;
    }

    void Core::setInputVolume(int volume) {
        if (m_audioEngine) m_audioEngine->setInputVolume(volume);
    }

    void Core::setOutputVolume(int volume) {
        if (m_audioEngine) m_audioEngine->setOutputVolume(volume);
    }

    bool Core::setInputDevice(int deviceIndex) {
        return m_audioEngine ? m_audioEngine->setInputDevice(deviceIndex) : false;
    }

    bool Core::setOutputDevice(int deviceIndex) {
        return m_audioEngine ? m_audioEngine->setOutputDevice(deviceIndex) : false;
    }

    int Core::getCurrentInputDevice() const {
        return m_audioEngine ? m_audioEngine->getCurrentInputDevice() : -1;
    }

    int Core::getCurrentOutputDevice() const {
        return m_audioEngine ? m_audioEngine->getCurrentOutputDevice() : -1;
    }

    bool Core::isAuthorized() const {
        return m_stateManager ? m_stateManager->isAuthorized() : false;
    }

    bool Core::isOutgoingCall() const {
        return m_stateManager ? m_stateManager->isOutgoingCall() : false;
    }

    bool Core::isActiveCall() const {
        return m_stateManager ? m_stateManager->isActiveCall() : false;
    }

    bool Core::isConnectionDown() const {
        return m_stateManager ? m_stateManager->isConnectionDown() : true;
    }

    int Core::getIncomingCallsCount() const {
        return m_stateManager ? m_stateManager->getIncomingCallsCount() : 0;
    }

    const std::string& Core::getMyNickname() const {
        static const std::string empty;
        return m_stateManager ? m_stateManager->getMyNickname() : empty;
    }

    const std::string& Core::getNicknameWhomOutgoingCall() const {
        static const std::string empty;
        return m_stateManager ? m_stateManager->getOutgoingCall().getNickname() : empty;
    }

    const std::string& Core::getNicknameInCallWith() const {
        static const std::string empty;
        return m_stateManager ? m_stateManager->getActiveCall().getNickname() : empty;
    }

    std::error_code Core::authorize(const std::string& nickname) {
        return m_authorizationService->authorize(nickname);
    }

    std::error_code Core::logout() {
        return m_authorizationService->logout();
    }

    std::error_code Core::startOutgoingCall(const std::string& userNickname) {
        return m_callService->startOutgoingCall(userNickname);
    }

    std::error_code Core::stopOutgoingCall() {
        return m_callService->stopOutgoingCall();
    }

    std::error_code Core::acceptCall(const std::string& userNickname) {
        auto ec = m_callService->acceptCall(userNickname);
        if (!ec && m_mediaService) {
            m_mediaService->startAudioSharing();
        }
        return ec;
    } 

    std::error_code Core::declineCall(const std::string& userNickname) {
        return m_callService->declineCall(userNickname);
    }

    std::error_code Core::endCall() {
        if (m_mediaService && m_stateManager->isActiveCall()) {
            (void)m_mediaService->stopScreenSharing(m_stateManager->getMyNickname(), m_stateManager->getActiveCall().getNickname());
            (void)m_mediaService->stopCameraSharing(m_stateManager->getMyNickname(), m_stateManager->getActiveCall().getNickname());
            m_mediaService->stopAudioSharing();
        }
        auto ec = m_callService->endCall();
        return ec;
    }

    std::error_code Core::startScreenSharing(const media::ScreenCaptureTarget& target) {
        if (!m_stateManager->isActiveCall()) return core::constant::make_error_code(core::constant::ErrorCode::no_active_call);

        return m_mediaService->startScreenSharing(m_stateManager->getMyNickname(), m_stateManager->getActiveCall().getNickname(), target);
    }

    std::error_code Core::stopScreenSharing() {
        if (!m_stateManager->isActiveCall()) return core::constant::make_error_code(core::constant::ErrorCode::no_active_call);

        return m_mediaService->stopScreenSharing(m_stateManager->getMyNickname(), m_stateManager->getActiveCall().getNickname());
    }

    std::error_code Core::startCameraSharing(std::string deviceName) {
        if (!m_stateManager->isActiveCall()) return core::constant::make_error_code(core::constant::ErrorCode::no_active_call);

        return m_mediaService->startCameraSharing(m_stateManager->getMyNickname(), m_stateManager->getActiveCall().getNickname(), deviceName);
    }

    std::error_code Core::stopCameraSharing() {
        if (!m_stateManager->isActiveCall()) return core::constant::make_error_code(core::constant::ErrorCode::no_active_call);

        return m_mediaService->stopCameraSharing(m_stateManager->getMyNickname(), m_stateManager->getActiveCall().getNickname());
    }

    bool Core::isCameraAvailable() const {
        return m_mediaService && m_mediaService->hasCameraAvailable();
    }
}