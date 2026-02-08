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
#include "eventListener.h"

#include "json.hpp"

namespace core::logic
{
    class MediaService {
    public:
        MediaService(
            std::shared_ptr<ClientStateManager> stateManager,
            std::shared_ptr<media::AudioEngine> audioEngine,
            std::shared_ptr<media::MediaProcessingService> mediaProcessingService,
            std::shared_ptr<EventListener> eventListener,
            std::function<void(const std::vector<unsigned char>&, core::constant::PacketType)> sendPacket,
            std::function<void(const std::vector<unsigned char>&, media::MediaType)> sendMediaFrame
        );

        ~MediaService() = default;

        std::error_code startScreenSharing(const std::string& myNickname, const std::string& userNickname, int screeIndex = 0);
        std::error_code stopScreenSharing(const std::string& myNickname, const std::string& userNickname);

        std::error_code startCameraSharing(const std::string& myNickname, const std::string& userNickname, std::string deviceName = "");
        std::error_code stopCameraSharing(const std::string& myNickname, const std::string& userNickname);

        std::error_code startAudioSharing();
        std::error_code stopAudioSharing();
            
    private:
        void onRawAudio(const float* data, int length);
        void onRawFrame(const media::Frame& frame);
        std::vector<unsigned char> processVideoFrame(media::MediaType type, const std::vector<unsigned char>& frameData);
        std::vector<unsigned char> processAudioFrame(const std::vector<float>& audioData);

    private:
        mutable std::mutex m_mutex;
        media::ScreenCaptureService m_screenCaptureService;
        media::CameraCaptureService m_cameraCaptureService;
        std::shared_ptr<ClientStateManager> m_stateManager;
        std::shared_ptr< media::MediaProcessingService> m_mediaProcessingService;
        std::shared_ptr<media::AudioEngine> m_audioEngine;
        std::shared_ptr<EventListener> m_eventListener;
        std::function<void(const std::vector<unsigned char>&, core::constant::PacketType)> m_sendPacket;
        std::function<void(const std::vector<unsigned char>&, media::MediaType)> m_sendMediaFrame;
    };
}
