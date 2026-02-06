#include "mediaService.h"
#include "packetFactory.h"
#include "utilities/crypto.h"

namespace core
{
    MediaService::MediaService(
        std::shared_ptr<ClientStateManager> stateManager,
        std::shared_ptr<EventListener> eventListener,
        std::function<void(const std::vector<unsigned char>&, PacketType)> sendPacket,
        std::function<void(const std::vector<unsigned char>&, media::MediaType)> sendMediaFrame)
        : m_stateManager(stateManager),
        m_eventListener(eventListener),
        m_sendPacket(sendPacket),
        m_sendMediaFrame(sendMediaFrame)
    {
        m_audioEngine.initialize([this](const unsigned char* data, int length) {onRawAudio(data, length); });
        m_screenCaptureService.setFrameCallback([this](const media::Frame& frame) {onRawFrame(frame); });
        m_cameraCaptureService.setFrameCallback([this](const media::Frame& frame) {onRawFrame(frame); });
        m_mediaProcessingService.initializeAudio();
        m_mediaProcessingService.initializeVideo();
    }

    std::error_code MediaService::startScreenSharing(const std::string& myNickname, const std::string& userNickname, int screeIndex = 0)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (m_stateManager->getMediaState(media::MediaType::Screen) != media::MediaState::Stopped) {
            return ; // TODO custom error code
        }

        m_stateManager->setMediaState(media::MediaType::Screen, media::MediaState::Starting);

        auto packet = PacketFactory::getTwoNicknamesPacket(myNickname, userNickname);
        m_sendPacket(packet, PacketType::SCREEN_SHARING_BEGIN);

        m_screenCaptureService.start(screeIndex);

        m_stateManager->setMediaState(media::MediaType::Screen, media::MediaState::Active);
    
        return {};
    }

    std::error_code MediaService::stopScreenSharing(const std::string& myNickname, const std::string& userNickname)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (m_stateManager->getMediaState(media::MediaType::Screen) != media::MediaState::Active) {
            return ; // TODO custom error code
        }

        auto packet = PacketFactory::getTwoNicknamesPacket(myNickname, userNickname);
        m_sendPacket(packet, PacketType::SCREEN_SHARING_END);

        m_screenCaptureService.stop();

        m_stateManager->setMediaState(media::MediaType::Screen, media::MediaState::Stopped);
    
        return {};
    }

    std::error_code MediaService::startCameraSharing(const std::string& myNickname, const std::string& userNickname, std::string deviceName = "")
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (m_stateManager->getMediaState(media::MediaType::Camera) != media::MediaState::Stopped) {
            return ; // TODO custom error code
        }

        m_stateManager->setMediaState(media::MediaType::Camera, media::MediaState::Starting);

        auto packet = PacketFactory::getTwoNicknamesPacket(myNickname, userNickname);
        m_sendPacket(packet, PacketType::CAMERA_SHARING_BEGIN);

        m_cameraCaptureService.start(deviceName);

        m_stateManager->setMediaState(media::MediaType::Camera, media::MediaState::Active);
    
        return {};
    }

    std::error_code MediaService::stopCameraSharing(const std::string& myNickname, const std::string& userNickname)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (m_stateManager->getMediaState(media::MediaType::Camera) != media::MediaState::Active) {
            return ; // TODO custom error code
        }

        auto packet = PacketFactory::getTwoNicknamesPacket(myNickname, userNickname);
        m_sendPacket(packet, PacketType::CAMERA_SHARING_END);

        m_cameraCaptureService.stop();

        m_stateManager->setMediaState(media::MediaType::Camera, media::MediaState::Stopped);

        return {};
    }

    std::error_code MediaService::startAudioSharing()
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (m_stateManager->getMediaState(media::MediaType::Audio) != media::MediaState::Stopped) {
            return ; // TODO custom error code
        }

        m_stateManager->setMediaState(media::MediaType::Audio, media::MediaState::Starting);

        m_audioEngine.startStream();

        m_stateManager->setMediaState(media::MediaType::Audio, media::MediaState::Active);

        return {};
    }

    std::error_code MediaService::stopAudioSharing()
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (m_stateManager->getMediaState(media::MediaType::Audio) != media::MediaState::Active) {
            return ; // TODO custom error code
        }

        m_audioEngine.stopStream();

        m_stateManager->setMediaState(media::MediaType::Audio, media::MediaState::Stopped);

        return {};
    }

    void MediaService::onIncomingScreenSharingStarted(const nlohmann::json& packet) {

    }

    void MediaService::onIncomingScreenSharingStopped(const nlohmann::json& packet) {

    }

    void MediaService::onIncomingCameraSharingStarted(const nlohmann::json& packet) {
        std::string senderNicknameHash = jsonObject[SENDER_NICKNAME_HASH];
        std::string myNicknameHash = jsonObject[RECEIVER_NICKNAME_HASH];

        if (!m_stateManager->isAuthorized() ||
            m_stateManager->isConnectionDown() ||
            !m_stateManager->isActiveCall() ||
            m_stateManager->isViewingRemoteScreen()) return;

        if (utilities::crypto::calculateHash(m_stateManager->getActiveCall().getNickname()) != senderNicknameHash ||
            utilities::crypto::calculateHash(m_stateManager->getMyNickname()) != myNicknameHash) return;

        m_stateManager->setViewingRemoteScreen(true);

        m_eventListener->onIncomingScreenSharingStarted();
    }

    void MediaService::onIncomingCameraSharingStoped(const nlohmann::json& packet) {

    }

    void MediaService::onIncomingAudio(const unsigned char* data, int length) {

    }

    void MediaService::onIncomingScreen(const unsigned char* data, int length) {

    }

    void MediaService::onIncomingCamera(const unsigned char* data, int length) {

    }

    void MediaService::setEncryptionKey(const std::vector<unsigned char>& key) {
        m_mediaProcessingService.setEncryptionKey(key);
    }
}
    
    //TODO create implementations and all necessary transformations of data and use m_eventListener onLocalScreen / onLocalCamera 

    void MediaService::onRawAudio(const unsigned char* data, int length) {
        std::lock_guard<std::mutex> lock(m_mutex);

    }

    void MediaService::onRawFrame(const media::Frame& frame) {
        std::lock_guard<std::mutex> lock(m_mutex);
       
    }

    std::vector<unsigned char> MediaService::processVideoFrame(MediaType type, const std::vector<unsigned char>& frameData)
    {
        if (!m_videoProcessor || !m_videoProcessor->isInitialized()) {
            return {};
        }

        // Encode frame to H.264
        auto encodedData = m_videoProcessor->encodeFrame(frameData.data(), 1920, 1080);
        if (encodedData.empty()) {
            return {};
        }

        // Encrypt if key is available
        if (m_stateManager.getMyKey().size_in_bytes() > 0) {
            return m_encryption->encryptMedia(encodedData.data(), encodedData.size(), m_stateManager.getMyKey());
        }

        return encodedData;
    }

    std::vector<unsigned char> MediaService::processAudioFrame(const std::vector<float>& audioData)
    {
        if (!m_audioProcessor || !m_audioProcessor->isInitialized()) {
            return {};
        }

        // Encode audio to Opus
        auto encodedData = m_audioProcessor->encodeFrame(audioData.data(), 960);
        if (encodedData.empty()) {
            return {};
        }

        // Encrypt if key is available
        if (m_stateManager.getMyKey().size_in_bytes() > 0) {
            return m_encryption->encryptMedia(encodedData.data(), encodedData.size(), m_stateManager.getMyKey());
        }

        return encodedData;
    }

