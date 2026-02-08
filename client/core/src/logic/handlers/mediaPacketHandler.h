#pragma once

#include <memory>
#include "media/audio/audioEngine.h"
#include "media/processing/mediaProcessingService.h"
#include "eventListener.h"

#include "json.hpp"

namespace core::logic
{
    class ClientStateManager;

    class MediaPacketHandler {
    public:
        MediaPacketHandler(
            std::shared_ptr<ClientStateManager> stateManager,
            std::shared_ptr<media::AudioEngine> audioEngine,
            std::shared_ptr<media::MediaProcessingService> mediaProcessingService,
            std::shared_ptr<EventListener> eventListener
        );

        void onIncomingScreenSharingStarted(const nlohmann::json& jsonObject);
        void onIncomingScreenSharingStopped(const nlohmann::json& jsonObject);
        void onIncomingCameraSharingStarted(const nlohmann::json& jsonObject);
        void onIncomingCameraSharingStoped(const nlohmann::json& jsonObject);
        void onIncomingAudio(const unsigned char* data, int length);
        void onIncomingScreen(const unsigned char* data, int length);
        void onIncomingCamera(const unsigned char* data, int length);

    private:
        std::shared_ptr<ClientStateManager> m_stateManager;
        std::shared_ptr<media::AudioEngine> m_audioEngine;
        std::shared_ptr<media::MediaProcessingService> m_mediaProcessingService;
        std::shared_ptr<EventListener> m_eventListener;
    }
}