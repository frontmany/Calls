#pragma once

#include <memory>
#include <map>
#include <string>
#include <chrono>
#include <functional>
#include <system_error>
#include <vector>
#include <cstdint>
#include "media/audio/audioEngine.h"
#include "media/processing/mediaProcessingService.h"
#include "eventListener.h"
#include "constants/packetType.h"

#include <nlohmann/json.hpp>

namespace core::logic
{
    class ClientStateManager;

    struct RemoteParticipantSpeakingState {
        bool speaking = false;
        int silenceCount = 0;
        float smoothedRms = 0.f;
    };

    struct NetworkStreamMetrics {
        bool initialized = false;
        uint32_t lastSeq = 0;
        uint64_t received = 0;
        uint64_t lost = 0;
        uint64_t bytes = 0;
        int warmupFramesLeft = 30;
        bool hasTiming = false;
        uint32_t lastRemoteTsMs = 0;
        std::chrono::steady_clock::time_point lastArrival{};
        double jitterMs = 0.0;
    };

    class MediaPacketHandler {
    public:
        MediaPacketHandler(
            std::shared_ptr<ClientStateManager> stateManager,
            std::shared_ptr<media::AudioEngine> audioEngine,
            std::shared_ptr<media::MediaProcessingService> mediaProcessingService,
            std::shared_ptr<EventListener> eventListener,
            std::function<std::error_code(const std::vector<unsigned char>&, core::constant::PacketType)> sendPacket
        );

        void handleIncomingScreenSharingStarted(const nlohmann::json& jsonObject);
        void handleIncomingScreenSharingStopped(const nlohmann::json& jsonObject);
        void handleIncomingCameraSharingStarted(const nlohmann::json& jsonObject);
        void handleIncomingCameraSharingStopped(const nlohmann::json& jsonObject);
        void handleIncomingAudio(const unsigned char* data, int length);
        void handleIncomingScreen(const unsigned char* data, int length);
        void handleIncomingCamera(const unsigned char* data, int length);
        void handleAdaptCommand(const nlohmann::json& jsonObject);
        void handleRttPong(const nlohmann::json& jsonObject);

    private:
        void updateMetricsFromFrame(const std::string& streamKey, uint32_t frameSeq, uint32_t timestampMs, int payloadLen);
        void sendStatsIfNeeded();
        void sendRttPingIfNeeded();

        std::shared_ptr<ClientStateManager> m_stateManager;
        std::shared_ptr<media::AudioEngine> m_audioEngine;
        std::shared_ptr<media::MediaProcessingService> m_mediaProcessingService;
        std::shared_ptr<EventListener> m_eventListener;
        std::map<std::string, RemoteParticipantSpeakingState> m_remoteParticipantSpeakingState;
        std::map<std::string, NetworkStreamMetrics> m_streamMetrics;
        std::function<std::error_code(const std::vector<unsigned char>&, core::constant::PacketType)> m_sendPacket;
        std::chrono::steady_clock::time_point m_lastStatsSentAt{};
        std::chrono::steady_clock::time_point m_lastPingSentAt{};
        uint64_t m_nextPingId = 1;
        std::map<uint64_t, std::chrono::steady_clock::time_point> m_pendingPings;
        int m_lastRttMs = 0;
    };
}