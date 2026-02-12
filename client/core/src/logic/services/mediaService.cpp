#include "mediaService.h"
#include "logic/packetFactory.h"
#include "constants/errorCode.h"
#include "utilities/logger.h"

using namespace core::constant;
using namespace core::media;

namespace core::logic
{
    MediaService::MediaService(
        std::shared_ptr<ClientStateManager> stateManager,
        std::shared_ptr<media::AudioEngine> audioEngine,
        std::shared_ptr<media::MediaProcessingService> mediaProcessingService,
        std::shared_ptr<EventListener> eventListener,
        std::function<std::error_code(const std::vector<unsigned char>&, PacketType)> sendPacket,
        std::function<std::error_code(const std::vector<unsigned char>&, core::constant::PacketType)> sendMediaFrame)
        : m_stateManager(stateManager)
        , m_audioEngine(audioEngine)
        , m_mediaProcessingService(mediaProcessingService)
        , m_eventListener(eventListener)
        , m_sendPacket(sendPacket)
        , m_sendMediaFrame(sendMediaFrame)
    {
        m_audioEngine->setInputAudioCallback([this](const float* data, int length) { onRawAudio(data, length); });
        m_screenCaptureService.setFrameCallback([this](const media::Frame& frame) { onRawFrame(frame, MediaType::Screen); });
        m_cameraCaptureService.setFrameCallback([this](const media::Frame& frame) { onRawFrame(frame, MediaType::Camera); });
    }

    std::error_code MediaService::startScreenSharing(const std::string& myNickname, const std::string& userNickname, const media::ScreenCaptureTarget& target)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (m_stateManager->getMediaState(MediaType::Screen) != MediaState::Stopped) {
            return make_error_code(ErrorCode::screen_sharing_already_active);
        }

        m_stateManager->setMediaState(MediaType::Screen, MediaState::Starting);

        auto packet = PacketFactory::getTwoNicknamesPacket(myNickname, userNickname);
        m_sendPacket(packet, PacketType::SCREEN_SHARING_BEGIN);

        if (!m_screenCaptureService.start(target)) {
            m_stateManager->setMediaState(MediaType::Screen, MediaState::Stopped);
            return make_error_code(ErrorCode::network_error);
        }

        m_stateManager->setMediaState(MediaType::Screen, MediaState::Active);
    
        return {};
    }

    std::error_code MediaService::stopScreenSharing(const std::string& myNickname, const std::string& userNickname)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (m_stateManager->getMediaState(MediaType::Screen) != MediaState::Active) {
            return make_error_code(ErrorCode::screen_sharing_not_active);
        }

        m_screenCaptureService.stop();

        auto packet = PacketFactory::getTwoNicknamesPacket(myNickname, userNickname);
        m_sendPacket(packet, PacketType::SCREEN_SHARING_END);

        m_stateManager->setMediaState(MediaType::Screen, MediaState::Stopped);
    
        return {};
    }

    std::error_code MediaService::startCameraSharing(const std::string& myNickname, const std::string& userNickname, std::string deviceName)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (m_stateManager->getMediaState(MediaType::Camera) != MediaState::Stopped) {
            return make_error_code(ErrorCode::camera_sharing_already_active);
        }

        m_stateManager->setMediaState(MediaType::Camera, MediaState::Starting);

        auto packet = PacketFactory::getTwoNicknamesPacket(myNickname, userNickname);
        m_sendPacket(packet, PacketType::CAMERA_SHARING_BEGIN);

        m_cameraCaptureService.start(deviceName.empty() ? nullptr : deviceName.c_str());

        m_stateManager->setMediaState(MediaType::Camera, MediaState::Active);
    
        return {};
    }

    std::error_code MediaService::stopCameraSharing(const std::string& myNickname, const std::string& userNickname)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (m_stateManager->getMediaState(MediaType::Camera) != MediaState::Active) {
            return make_error_code(ErrorCode::camera_sharing_not_active);
        }

        m_cameraCaptureService.stop();

        auto packet = PacketFactory::getTwoNicknamesPacket(myNickname, userNickname);
        m_sendPacket(packet, PacketType::CAMERA_SHARING_END);

        m_stateManager->setMediaState(MediaType::Camera, MediaState::Stopped);

        return {};
    }

    std::error_code MediaService::startAudioSharing()
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (m_stateManager->getMediaState(MediaType::Audio) != MediaState::Stopped) {
            return make_error_code(ErrorCode::operation_in_progress);
        }

        m_stateManager->setMediaState(MediaType::Audio, MediaState::Starting);

        if (!m_audioEngine->startAudioCapture()) {
            m_stateManager->setMediaState(MediaType::Audio, MediaState::Stopped);
            return make_error_code(ErrorCode::network_error);
        }

        m_stateManager->setMediaState(MediaType::Audio, MediaState::Active);

        return {};
    }

    std::error_code MediaService::stopAudioSharing()
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (m_stateManager->getMediaState(MediaType::Audio) != MediaState::Active) {
            return make_error_code(ErrorCode::operation_in_progress);
        }

        m_audioEngine->stopAudioCapture();

        m_stateManager->setMediaState(MediaType::Audio, MediaState::Stopped);

        return {};
    }

    void MediaService::onRawAudio(const float* data, int length) {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (!m_stateManager->isActiveCall() ||
            m_stateManager->getMediaState(MediaType::Audio) != MediaState::Active) {
            return;
        }

        auto encodedAudio = m_mediaProcessingService->encodeAudioFrame(data, length);
        if (encodedAudio.empty()) {
            return;
        }

        auto encryptedAudio = encryptWithCallKey(encodedAudio);
        if (encryptedAudio.empty()) {
            return;
        }

        m_sendMediaFrame(encryptedAudio, PacketType::VOICE);
    }

    void MediaService::onRawFrame(const media::Frame& frame, MediaType type) {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (!frame.isValid() || !m_stateManager->isActiveCall()) {
            return;
        }

        if (m_stateManager->getMediaState(type) != MediaState::Active) {
            return;
        }

        // Send local preview to UI
        if (m_eventListener) {
            std::vector<unsigned char> rawData(frame.data, frame.data + frame.size);
            if (type == MediaType::Screen) {
                m_eventListener->onLocalScreen(rawData, frame.width, frame.height);
            } else {
                m_eventListener->onLocalCamera(rawData, frame.width, frame.height);
            }
        }

        // Encode, encrypt, send
        auto encodedVideo = m_mediaProcessingService->encodeVideoFrame(frame.data, frame.width, frame.height);
        if (encodedVideo.empty()) {
            return;
        }

        auto encryptedVideo = encryptWithCallKey(encodedVideo);
        if (encryptedVideo.empty()) {
            return;
        }

        PacketType packetType = (type == MediaType::Screen) ? PacketType::SCREEN : PacketType::CAMERA;
        m_sendMediaFrame(encryptedVideo, packetType);
    }

    std::vector<unsigned char> MediaService::encryptWithCallKey(const std::vector<unsigned char>& data) {
        if (!m_stateManager->isActiveCall()) {
            return {};
        }

        const auto& callKey = m_stateManager->getActiveCall().getCallKey();
        if (callKey.size() == 0) {
            return data;
        }

        std::vector<unsigned char> keyVec(callKey.begin(), callKey.end());
        return m_mediaProcessingService->encryptData(data.data(), static_cast<int>(data.size()), keyVec);
    }
}
