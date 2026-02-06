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
#include "utilities/packetType.h"
#include "eventListener.h"

#include "json.hpp"

namespace core
{
    class MediaService {
    public:
        MediaService(
            std::shared_ptr<ClientStateManager> stateManager,
            std::shared_ptr<EventListener> eventListener,
            std::function<void(const std::vector<unsigned char>&, PacketType)> sendPacket,
            std::function<void(const std::vector<unsigned char>&, media::MediaType)> sendMediaFrame
        );

        ~MediaService() = default;

        std::error_code startScreenSharing(const std::string& myNickname, const std::string& userNickname, int screeIndex = 0);
        std::error_code stopScreenSharing(const std::string& myNickname, const std::string& userNickname);

        std::error_code startCameraSharing(const std::string& myNickname, const std::string& userNickname, std::string deviceName = "");
        std::error_code stopCameraSharing(const std::string& myNickname, const std::string& userNickname);

        std::error_code startAudioSharing();
        std::error_code stopAudioSharing();

        void onIncomingScreenSharingStarted(const nlohmann::json& packet);
        void onIncomingScreenSharingStopped(const nlohmann::json& packet);
        void onIncomingCameraSharingStarted(const nlohmann::json& packet);
        void onIncomingCameraSharingStoped(const nlohmann::json& packet);
        void onIncomingAudio(const unsigned char* data, int length);
        void onIncomingScreen(const unsigned char* data, int length);
        void onIncomingCamera(const unsigned char* data, int length);

        void setEncryptionKey(const std::vector<unsigned char>& key);
            
    private:
        void onRawAudio(const unsigned char* data, int length);
        void onRawFrame(const media::Frame& frame);
        std::vector<unsigned char> processVideoFrame(media::MediaType type, const std::vector<unsigned char>& frameData);
        std::vector<unsigned char> processAudioFrame(const std::vector<float>& audioData);

    private:
        mutable std::mutex m_mutex;
        media::AudioEngine m_audioEngine;
        media::MediaProcessingService m_mediaProcessingService;
        media::ScreenCaptureService m_screenCaptureService;
        media::CameraCaptureService m_cameraCaptureService;
        std::shared_ptr<ClientStateManager> m_stateManager;
        std::shared_ptr<EventListener> m_eventListener;
        std::function<void(const std::vector<unsigned char>&, PacketType)> m_sendPacket;
        std::function<void(const std::vector<unsigned char>&, media::MediaType)> m_sendMediaFrame;
    };
}
