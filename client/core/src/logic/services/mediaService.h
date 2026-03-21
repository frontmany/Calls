#pragma once

#include <vector>
#include <memory>
#include <cstdint>
#include <functional>
#include <system_error>

#include "media/mediaType.h"
#include "media/mediaState.h"
#include "media/audio/audioEngine.h"
#include "media/screen/screenCaptureService.h"
#include "media/camera/cameraCaptureService.h"
#include "media/processing/mediaProcessingService.h"
#include "logic/clientStateManager.h"
#include "constants/packetType.h"
#include "constants/speakingVad.h"
#include "eventListener.h"

#include <nlohmann/json.hpp>

namespace core::logic
{
    class MediaService {
    public:
        MediaService(
            std::shared_ptr<ClientStateManager> stateManager,
            std::shared_ptr<media::AudioEngine> audioEngine,
            std::shared_ptr<media::MediaProcessingService> mediaProcessingService,
            std::shared_ptr<EventListener> eventListener,
            std::function<std::error_code(const std::vector<unsigned char>&, core::constant::PacketType)> sendPacket,
            std::function<std::error_code(const std::vector<unsigned char>&, core::constant::PacketType)> sendMediaFrame
        );

        ~MediaService() = default;

        std::error_code startScreenSharing(const std::string& myNickname, const std::string& userNickname, const media::Screen& target);
        std::error_code stopScreenSharing(const std::string& myNickname, const std::string& userNickname);

        std::error_code startCameraSharing(const std::string& myNickname, const std::string& userNickname, std::string deviceName = "");
        std::error_code stopCameraSharing(const std::string& myNickname, const std::string& userNickname);
        bool hasCameraAvailable() const;
        std::vector<media::Camera> getCameraDevices() const;
        std::vector<media::Screen> getScreens() const;

        std::error_code startAudioSharing();
        std::error_code stopAudioSharing();
        void onMicrophoneMuteChanged(bool isMuted);
            
    private:
        void onRawAudio(const float* data, int length);
        void onRawFrame(const media::Frame& frame, media::MediaType type);
        std::vector<unsigned char> encryptWithCallKey(const std::vector<unsigned char>& data);
        std::vector<unsigned char> buildMeetingFrame(const std::string& meetingId,
            const std::string& senderHash,
            const std::vector<unsigned char>& encryptedPayload);

    private:
        mutable std::mutex m_mutex;
        media::ScreenCaptureService m_screenCaptureService;
        media::CameraCaptureService m_cameraCaptureService;
        std::shared_ptr<ClientStateManager> m_stateManager;
        std::shared_ptr< media::MediaProcessingService> m_mediaProcessingService;
        std::shared_ptr<media::AudioEngine> m_audioEngine;
        std::shared_ptr<EventListener> m_eventListener;
        std::function<std::error_code(const std::vector<unsigned char>&, core::constant::PacketType)> m_sendPacket;
        std::function<std::error_code(const std::vector<unsigned char>&, core::constant::PacketType)> m_sendMediaFrame;

        bool m_localParticipantSpeaking = false;
        int m_silenceFramesCount = 0;
        float m_localSmoothedRms = 0.f;
    };
}