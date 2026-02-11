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

        void handleIncomingScreenSharingStarted(const nlohmann::json& jsonObject);
        void handleIncomingScreenSharingStopped(const nlohmann::json& jsonObject);
        void handleIncomingCameraSharingStarted(const nlohmann::json& jsonObject);
        void handleIncomingCameraSharingStopped(const nlohmann::json& jsonObject);
        void handleIncomingAudio(const unsigned char* data, int length);
        void handleIncomingScreen(const unsigned char* data, int length);
        void handleIncomingCamera(const unsigned char* data, int length);

    private:
        std::shared_ptr<ClientStateManager> m_stateManager;
        std::shared_ptr<media::AudioEngine> m_audioEngine;
        std::shared_ptr<media::MediaProcessingService> m_mediaProcessingService;
        std::shared_ptr<EventListener> m_eventListener;
    };
}