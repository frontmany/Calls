#include "mediaService.h"
#include "logic/packetFactory.h"
#include "constants/errorCode.h"
#include "utilities/logger.h"
#include "utilities/crypto.h"

#include <cstdint>
#include <optional>

using namespace core::constant;
using namespace core::media;

namespace core::logic
{
    namespace
    {
        std::optional<std::vector<unsigned char>> deriveMeetingKeyVec(const std::string& meetingId)
        {
            auto binOpt = core::utilities::crypto::hashToBinary(core::utilities::crypto::calculateHash(meetingId));
            if (!binOpt) {
                return std::nullopt;
            }
            return std::vector<unsigned char>(binOpt->begin(), binOpt->end());
        }

        void appendU16BE(std::vector<unsigned char>& out, uint16_t value)
        {
            out.push_back(static_cast<unsigned char>((value >> 8) & 0xFF));
            out.push_back(static_cast<unsigned char>(value & 0xFF));
        }
    }

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
        m_cachedMyNickname.clear();
        m_cachedMyNicknameHash.clear();
        m_audioEngine->setInputAudioCallback([this](const float* data, int length) { onRawAudio(data, length); });
        m_screenCaptureService.setFrameCallback([this](const media::Frame& frame) { onRawFrame(frame, MediaType::Screen); });
        m_cameraCaptureService.setFrameCallback([this](const media::Frame& frame) { onRawFrame(frame, MediaType::Camera); });
        m_cameraCaptureService.setErrorCallback([this]() {
            // Called from camera thread when camera initialization fails.
            // Do NOT call stop() here - it would deadlock (stop joins the thread we're in).
            // UI will call stopCameraSharing via onStartCameraSharingError to clean up.
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                if (m_stateManager && m_stateManager->getMediaState(MediaType::Camera) != MediaState::Stopped) {
                    m_stateManager->setMediaState(MediaType::Camera, MediaState::Stopped);

                    if (m_stateManager->isActiveCall() || m_stateManager->isInMeeting()) {
                        auto packet = PacketFactory::getCameraSharingEndPacket(
                            m_stateManager->getMyNickname()
                        );
                        (void)m_sendPacket(packet, PacketType::CAMERA_SHARING_END);
                    }
                }
            }

            if (m_eventListener) {
                m_eventListener->onStartCameraSharingError();
            }
        });
    }

    std::error_code MediaService::startScreenSharing(const std::string& myNickname, const std::string& userNickname, const media::Screen& target)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (m_stateManager->getMediaState(MediaType::Screen) != MediaState::Stopped) {
            return make_error_code(ErrorCode::screen_sharing_already_active);
        }

        m_stateManager->setMediaState(MediaType::Screen, MediaState::Starting);

        auto packet = PacketFactory::getScreenSharingBeginPacket(myNickname);
        m_sendPacket(packet, PacketType::SCREEN_SHARING_BEGIN);

        constexpr int kScreenBitrate = 1800000;
        if (!m_mediaProcessingService->initializeVideoProcessing(MediaType::Screen, kScreenBitrate)) {
            m_stateManager->setMediaState(MediaType::Screen, MediaState::Stopped);
            return make_error_code(ErrorCode::network_error);
        }

        if (!m_screenCaptureService.start(target)) {
            m_stateManager->setMediaState(MediaType::Screen, MediaState::Stopped);
            return make_error_code(ErrorCode::network_error);
        }

        m_stateManager->setMediaState(MediaType::Screen, MediaState::Active);
    
        return {};
    }

    std::error_code MediaService::stopScreenSharing(const std::string& myNickname, const std::string& userNickname)
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_stateManager->getMediaState(MediaType::Screen) != MediaState::Active) {
                return make_error_code(ErrorCode::screen_sharing_not_active);
            }
            m_stateManager->setMediaState(MediaType::Screen, MediaState::Stopped);
        }
        // Do not hold m_mutex here: capture thread callback (onRawFrame) takes m_mutex; stop() joins that thread -> deadlock.
        m_screenCaptureService.stop();
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            // Do not cleanup Screen pipeline here: decoder is needed to display remote's screen when they share.
            // Pipeline is reinitialized on next startScreenSharing().
            auto packet = PacketFactory::getScreenSharingEndPacket(myNickname);
            m_sendPacket(packet, PacketType::SCREEN_SHARING_END);
        }
        return {};
    }

    std::error_code MediaService::startCameraSharing(const std::string& myNickname, const std::string& userNickname, std::string deviceName)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (m_stateManager->getMediaState(MediaType::Camera) != MediaState::Stopped) {
            return make_error_code(ErrorCode::camera_sharing_already_active);
        }

        m_stateManager->setMediaState(MediaType::Camera, MediaState::Starting);

        auto packet = PacketFactory::getCameraSharingBeginPacket(myNickname);
        m_sendPacket(packet, PacketType::CAMERA_SHARING_BEGIN);

        if (!m_cameraCaptureService.start(deviceName.empty() ? nullptr : deviceName.c_str())) {
            m_stateManager->setMediaState(MediaType::Camera, MediaState::Stopped);
            m_sendPacket(PacketFactory::getCameraSharingEndPacket(myNickname), PacketType::CAMERA_SHARING_END);
            return make_error_code(ErrorCode::network_error);
        }

        return {};
    }

    bool MediaService::hasCameraAvailable() const
    {
        char devices[10][256];
        int deviceCount = 0;
        return media::CameraCaptureService::getAvailableDevices(devices, 10, deviceCount) && deviceCount > 0;
    }

    std::vector<media::Camera> MediaService::getCameraDevices() const
    {
        return media::CameraCaptureService::getCameraDevices();
    }

    std::vector<media::Screen> MediaService::getScreens() const
    {
        return media::ScreenCaptureService::getAvailableTargets();
    }

    std::error_code MediaService::stopCameraSharing(const std::string& myNickname, const std::string& userNickname)
    {
        MediaState previousState = MediaState::Stopped;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_stateManager) {
                previousState = m_stateManager->getMediaState(MediaType::Camera);
                if (previousState != MediaState::Stopped) {
                    m_stateManager->setMediaState(MediaType::Camera, MediaState::Stopped);
                }
            }
        }
        m_cameraCaptureService.stop();

        if (previousState != MediaState::Stopped) {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto packet = PacketFactory::getCameraSharingEndPacket(myNickname);
            m_sendPacket(packet, PacketType::CAMERA_SHARING_END);
        }
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

        const bool isActiveCall = m_stateManager->isActiveCall();
        const bool isInMeeting = m_stateManager->isInMeeting();

        if ((!isActiveCall && !isInMeeting) ||
            m_stateManager->getMediaState(MediaType::Audio) != MediaState::Active) {
            if (m_localParticipantSpeaking && m_eventListener) {
                const std::string& myNick = m_stateManager->getMyNickname();
                if (!myNick.empty())
                    m_eventListener->onMeetingParticipantSpeaking(myNick, false);
            }
            m_localParticipantSpeaking = false;
            m_silenceFramesCount = 0;
            m_localSmoothedRms = 0.f;
            return;
        }

        auto encodedAudio = m_mediaProcessingService->encodeAudioFrame(data);
        if (encodedAudio.empty()) {
            return;
        }

        std::vector<unsigned char> encryptedAudio;
        if (isActiveCall) {
            encryptedAudio = encryptWithCallKey(encodedAudio);
        }
        else {
            auto meetingOpt = m_stateManager->getActiveMeeting();
            if (!meetingOpt) return;
            {
                const float rmsVal = core::constant::computeRms(data, length);
                m_localSmoothedRms = core::constant::smoothRms(m_localSmoothedRms, rmsVal);
                if (m_localSmoothedRms > core::constant::kSpeakingRmsThreshold) {
                    m_silenceFramesCount = 0;
                    if (!m_localParticipantSpeaking) {
                        m_localParticipantSpeaking = true;
                        const std::string& myNick = m_stateManager->getMyNickname();
                        if (m_eventListener && !myNick.empty())
                            m_eventListener->onMeetingParticipantSpeaking(myNick, true);
                    }
                } else {
                    m_silenceFramesCount++;
                    if (m_silenceFramesCount >= core::constant::kSpeakingSilenceFrames) {
                        m_silenceFramesCount = core::constant::kSpeakingSilenceFrames;
                        if (m_localParticipantSpeaking) {
                            m_localParticipantSpeaking = false;
                            const std::string& myNick = m_stateManager->getMyNickname();
                            if (m_eventListener && !myNick.empty())
                                m_eventListener->onMeetingParticipantSpeaking(myNick, false);
                        }
                    }
                }
            }
            const std::string& meetingId = meetingOpt->get().getMeetingId();
            auto keyOpt = deriveMeetingKeyVec(meetingId);
            if (!keyOpt) return;

            encryptedAudio = m_mediaProcessingService->encryptData(
                encodedAudio.data(),
                static_cast<int>(encodedAudio.size()),
                *keyOpt
            );

            if (encryptedAudio.empty()) {
                return;
            }

            if (meetingId.size() > 0xFFFF) {
                return;
            }

            const std::string& senderHash = getCachedMyNicknameHash();
            if (senderHash.size() > 0xFFFF) {
                return;
            }

            std::vector<unsigned char> framed = buildMeetingFrame(meetingId, senderHash, encryptedAudio);
            if (!framed.empty()) {
                m_sendMediaFrame(framed, PacketType::VOICE);
            }
            return;
        }

        if (encryptedAudio.empty()) {
            return;
        }

        m_sendMediaFrame(encryptedAudio, PacketType::VOICE);
    }

    void MediaService::onRawFrame(const media::Frame& frame, MediaType type) {
        std::lock_guard<std::mutex> lock(m_mutex);

        const bool isActiveCall = m_stateManager->isActiveCall();
        const bool isInMeeting = m_stateManager->isInMeeting();
        if (!frame.isValid() || (!isActiveCall && !isInMeeting)) {
            return;
        }

        if (type == MediaType::Camera && m_stateManager->getMediaState(MediaType::Camera) == MediaState::Starting) {
            m_stateManager->setMediaState(MediaType::Camera, MediaState::Active);
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
        auto encodedVideo = m_mediaProcessingService->encodeVideoFrame(type, frame.data, frame.width, frame.height);
        if (encodedVideo.empty()) {
            return;
        }

        PacketType packetType = (type == MediaType::Screen) ? PacketType::SCREEN : PacketType::CAMERA;
        if (isActiveCall) {
            auto encryptedVideo = encryptWithCallKey(encodedVideo);
            if (encryptedVideo.empty()) {
                return;
            }
            m_sendMediaFrame(encryptedVideo, packetType);
            return;
        }

        auto meetingOpt = m_stateManager->getActiveMeeting();
        if (!meetingOpt) return;
        const std::string& meetingId = meetingOpt->get().getMeetingId();
        auto keyOpt = deriveMeetingKeyVec(meetingId);
        if (!keyOpt) return;

        auto encryptedVideo = m_mediaProcessingService->encryptData(
            encodedVideo.data(),
            static_cast<int>(encodedVideo.size()),
            *keyOpt
        );
        if (encryptedVideo.empty()) {
            return;
        }

        if (meetingId.size() > 0xFFFF) {
            return;
        }

        const std::string& senderHash = getCachedMyNicknameHash();
        if (senderHash.size() > 0xFFFF) {
            return;
        }
        std::vector<unsigned char> framed = buildMeetingFrame(meetingId, senderHash, encryptedVideo);
        if (!framed.empty()) {
            m_sendMediaFrame(framed, packetType);
        }
    }

    std::vector<unsigned char> MediaService::buildMeetingFrame(const std::string& meetingId,
        const std::string& senderHash,
        const std::vector<unsigned char>& encryptedPayload)
    {
        std::vector<unsigned char> framed;
        framed.reserve(2 + meetingId.size() + 2 + senderHash.size() + encryptedPayload.size());
        appendU16BE(framed, static_cast<uint16_t>(meetingId.size()));
        framed.insert(framed.end(), meetingId.begin(), meetingId.end());
        appendU16BE(framed, static_cast<uint16_t>(senderHash.size()));
        framed.insert(framed.end(), senderHash.begin(), senderHash.end());
        framed.insert(framed.end(), encryptedPayload.begin(), encryptedPayload.end());
        return framed;
    }

    const std::string& MediaService::getCachedMyNicknameHash() {
        const std::string& nickname = m_stateManager->getMyNickname();
        if (m_cachedMyNickname != nickname) {
            m_cachedMyNickname = nickname;
            m_cachedMyNicknameHash = core::utilities::crypto::calculateHash(nickname);
        }
        return m_cachedMyNicknameHash;
    }

    std::vector<unsigned char> MediaService::encryptWithCallKey(const std::vector<unsigned char>& data) {
        auto activeOpt = m_stateManager->getActiveCall();
        if (!activeOpt) {
            return {};
        }

        const auto& callKey = activeOpt->get().getCallKey();
        if (callKey.size() == 0) {
            return data;
        }

        std::vector<unsigned char> keyVec(callKey.begin(), callKey.end());
        return m_mediaProcessingService->encryptData(data.data(), static_cast<int>(data.size()), keyVec);
    }
}
