#include "mediaPacketHandler.h"
#include "logic/clientStateManager.h"
#include "constants/jsonType.h"
#include "constants/speakingVad.h"
#include "media/mediaType.h"
#include "utilities/crypto.h"

#include <cstdint>
#include <optional>
#include <chrono>
#include <cmath>
#include <algorithm>

using namespace core::constant;
using namespace core::media;

namespace core::logic
{
    namespace
    {
        struct MeetingFrame {
            uint8_t version = 0;
            std::string meetingId;
            std::string senderHash;
            uint8_t mediaKind = 0;
            uint8_t layerId = 0;
            uint32_t frameSeq = 0;
            uint32_t timestampMs = 0;
            const unsigned char* payload = nullptr;
            int payloadLen = 0;
        };

        std::optional<MeetingFrame> parseMeetingFrame(const unsigned char* data, int length) {
            if (length < 1 + 2) return std::nullopt;
            const uint8_t version = data[0];
            if (version != 1) return std::nullopt;

            const size_t meetingLenOffset = 1;
            const uint16_t meetingIdLen = (static_cast<uint16_t>(data[meetingLenOffset]) << 8)
                | static_cast<uint16_t>(data[meetingLenOffset + 1]);
            if (length < static_cast<int>(meetingLenOffset + 2 + meetingIdLen + 2)) return std::nullopt;
            const size_t meetingIdOffset = meetingLenOffset + 2;
            const std::string meetingId(reinterpret_cast<const char*>(data + meetingIdOffset), meetingIdLen);

            const size_t senderLenOffset = meetingIdOffset + meetingIdLen;
            const uint16_t senderHashLen = (static_cast<uint16_t>(data[senderLenOffset]) << 8)
                | static_cast<uint16_t>(data[senderLenOffset + 1]);
            const size_t senderOffset = senderLenOffset + 2;
            if (length < static_cast<int>(senderOffset + senderHashLen + 1 + 1 + 4 + 4)) return std::nullopt;
            const std::string senderHash(reinterpret_cast<const char*>(data + senderOffset), senderHashLen);

            const size_t metaOffset = senderOffset + senderHashLen;
            const uint8_t mediaKind = data[metaOffset];
            const uint8_t layerId = data[metaOffset + 1];
            const uint32_t frameSeq = (static_cast<uint32_t>(data[metaOffset + 2]) << 24)
                | (static_cast<uint32_t>(data[metaOffset + 3]) << 16)
                | (static_cast<uint32_t>(data[metaOffset + 4]) << 8)
                | static_cast<uint32_t>(data[metaOffset + 5]);
            const uint32_t timestampMs = (static_cast<uint32_t>(data[metaOffset + 6]) << 24)
                | (static_cast<uint32_t>(data[metaOffset + 7]) << 16)
                | (static_cast<uint32_t>(data[metaOffset + 8]) << 8)
                | static_cast<uint32_t>(data[metaOffset + 9]);
            const unsigned char* payload = data + metaOffset + 10;
            const int payloadLen = length - static_cast<int>(metaOffset + 10);
            MeetingFrame frame;
            frame.version = version;
            frame.meetingId = meetingId;
            frame.senderHash = senderHash;
            frame.mediaKind = mediaKind;
            frame.layerId = layerId;
            frame.frameSeq = frameSeq;
            frame.timestampMs = timestampMs;
            frame.payload = payload;
            frame.payloadLen = payloadLen;
            return frame;
        }

        uint32_t elapsedMs(const std::chrono::steady_clock::time_point& from, const std::chrono::steady_clock::time_point& to)
        {
            return static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(to - from).count());
        }

        std::string makeStreamMetricsKey(const std::string& senderHash, const char* mediaKind, uint8_t layerId)
        {
            return senderHash + "|" + mediaKind + "|L" + std::to_string(static_cast<int>(layerId));
        }

        std::string makeCallMetricsKey(const std::string& senderHash, const char* mediaKind)
        {
            return senderHash + "|" + mediaKind + "|call";
        }

        bool isMeetingParticipantHash(
            const std::shared_ptr<ClientStateManager>& stateManager,
            const std::string& senderNicknameHash)
        {
            auto meetingOpt = stateManager->getActiveMeeting();
            if (!meetingOpt) return false;

            for (const auto& participant : meetingOpt->get().getParticipants()) {
                if (core::utilities::crypto::calculateHash(participant.getUser().getNickname()) == senderNicknameHash) {
                    return true;
                }
            }
            return false;
        }

        std::string meetingParticipantNicknameByHash(
            const std::shared_ptr<ClientStateManager>& stateManager,
            const std::string& senderNicknameHash)
        {
            auto meetingOpt = stateManager->getActiveMeeting();
            if (!meetingOpt) return {};

            for (const auto& participant : meetingOpt->get().getParticipants()) {
                if (core::utilities::crypto::calculateHash(participant.getUser().getNickname()) == senderNicknameHash) {
                    return participant.getUser().getNickname();
                }
            }
            return {};
        }
    }

    MediaPacketHandler::MediaPacketHandler(
        std::shared_ptr<ClientStateManager> stateManager,
        std::shared_ptr<media::AudioEngine> audioEngine,
        std::shared_ptr<media::MediaProcessingService> mediaProcessingService,
        std::shared_ptr<EventListener> eventListener,
        std::function<std::error_code(const std::vector<unsigned char>&, core::constant::PacketType)> sendPacket)
        : m_stateManager(stateManager)
        , m_audioEngine(audioEngine)
        , m_mediaProcessingService(mediaProcessingService)
        , m_eventListener(eventListener)
        , m_sendPacket(std::move(sendPacket))
    {
    }

    void MediaPacketHandler::handleIncomingScreenSharingStarted(const nlohmann::json& jsonObject) {
        if (!m_stateManager->isAuthorized() ||
            m_stateManager->isConnectionDown() ||
            m_stateManager->isViewingRemoteScreen()) return;

        const std::string& senderNicknameHash = jsonObject[SENDER_NICKNAME_HASH].get<std::string>();
        std::string sharerNickname;

        auto activeOpt = m_stateManager->getActiveCall();
        if (activeOpt) {
            if (utilities::crypto::calculateHash(activeOpt->get().getNickname()) != senderNicknameHash) return;
            sharerNickname = activeOpt->get().getNickname();
        } else {
            if (!m_stateManager->isActiveMeeting() || !isMeetingParticipantHash(m_stateManager, senderNicknameHash)) return;
            sharerNickname = meetingParticipantNicknameByHash(m_stateManager, senderNicknameHash);
        }

        m_stateManager->setViewingRemoteScreen(true);
        m_eventListener->onIncomingScreenSharingStarted(sharerNickname);
    }

    void MediaPacketHandler::handleIncomingScreenSharingStopped(const nlohmann::json& jsonObject) {
        if (!m_stateManager->isAuthorized() ||
            m_stateManager->isConnectionDown() ||
            !m_stateManager->isViewingRemoteScreen()) return;

        const std::string& senderNicknameHash = jsonObject[SENDER_NICKNAME_HASH].get<std::string>();
        std::string sharerNickname;

        auto activeOpt = m_stateManager->getActiveCall();
        if (activeOpt) {
            if (utilities::crypto::calculateHash(activeOpt->get().getNickname()) != senderNicknameHash) return;
            sharerNickname = activeOpt->get().getNickname();
        } else {
            if (!m_stateManager->isActiveMeeting() || !isMeetingParticipantHash(m_stateManager, senderNicknameHash)) return;
            sharerNickname = meetingParticipantNicknameByHash(m_stateManager, senderNicknameHash);
        }

        m_stateManager->setViewingRemoteScreen(false);
        m_eventListener->onIncomingScreenSharingStopped(sharerNickname);
    }

    void MediaPacketHandler::handleIncomingCameraSharingStarted(const nlohmann::json& jsonObject) {
        if (!m_stateManager->isAuthorized() ||
            m_stateManager->isConnectionDown()) return;

        const std::string& senderNicknameHash = jsonObject[SENDER_NICKNAME_HASH].get<std::string>();
        std::string nickname;

        auto activeOpt = m_stateManager->getActiveCall();
        if (activeOpt) {
            if (utilities::crypto::calculateHash(activeOpt->get().getNickname()) != senderNicknameHash) return;
            nickname = activeOpt->get().getNickname();
        } else {
            if (!m_stateManager->isActiveMeeting() || !isMeetingParticipantHash(m_stateManager, senderNicknameHash)) return;
            nickname = meetingParticipantNicknameByHash(m_stateManager, senderNicknameHash);
        }
        if (nickname.empty()) return;

        m_stateManager->addRemoteCameraSender(senderNicknameHash);
        m_eventListener->onIncomingCameraSharingStarted(nickname);
    }

    void MediaPacketHandler::handleIncomingCameraSharingStopped(const nlohmann::json& jsonObject) {
        if (!m_stateManager->isAuthorized() ||
            m_stateManager->isConnectionDown()) return;

        const std::string& senderNicknameHash = jsonObject[SENDER_NICKNAME_HASH].get<std::string>();
        std::string nickname;

        auto activeOpt = m_stateManager->getActiveCall();
        if (activeOpt) {
            if (utilities::crypto::calculateHash(activeOpt->get().getNickname()) != senderNicknameHash) return;
            nickname = activeOpt->get().getNickname();
        } else {
            if (!m_stateManager->isActiveMeeting() || !isMeetingParticipantHash(m_stateManager, senderNicknameHash)) return;
            nickname = meetingParticipantNicknameByHash(m_stateManager, senderNicknameHash);
        }
        if (nickname.empty()) return;

        m_stateManager->removeRemoteCameraSender(senderNicknameHash);
        m_eventListener->onIncomingCameraSharingStopped(nickname);
    }

    void MediaPacketHandler::handleIncomingAudio(const unsigned char* data, int length) {
        if (!m_stateManager->isAuthorized() ||
            m_stateManager->isConnectionDown() ||
            !m_audioEngine->isStream()) return;

        auto updateSpeakingState = [this](const std::string& nickname, const std::vector<float>& audioFrame, bool isCallContext) {
            if (nickname.empty() || !m_eventListener || audioFrame.empty()) {
                return;
            }

            const float rms = core::constant::computeRms(audioFrame.data(), static_cast<int>(audioFrame.size()));
            RemoteParticipantSpeakingState& state = m_remoteParticipantSpeakingState[nickname];
            state.smoothedRms = core::constant::smoothRms(state.smoothedRms, rms);

            if (state.smoothedRms > core::constant::kSpeakingRmsThreshold) {
                state.silenceCount = 0;
                if (!state.speaking) {
                    state.speaking = true;
                    if (isCallContext) {
                        m_eventListener->onCallParticipantSpeaking(nickname, true);
                    } else {
                        m_eventListener->onMeetingParticipantSpeaking(nickname, true);
                    }
                }
                return;
            }

            state.silenceCount++;
            if (state.silenceCount >= core::constant::kSpeakingSilenceFrames) {
                state.silenceCount = core::constant::kSpeakingSilenceFrames;
                if (state.speaking) {
                    state.speaking = false;
                    if (isCallContext) {
                        m_eventListener->onCallParticipantSpeaking(nickname, false);
                    } else {
                        m_eventListener->onMeetingParticipantSpeaking(nickname, false);
                    }
                }
            }
        };

        auto activeOpt = m_stateManager->getActiveCall();
        if (activeOpt) {
            auto frameOpt = parseMeetingFrame(data, length);
            const unsigned char* payload = data;
            int payloadLen = length;
            if (frameOpt && frameOpt->mediaKind == 0) {
                payload = frameOpt->payload;
                payloadLen = frameOpt->payloadLen;
                const std::string senderHash = core::utilities::crypto::calculateHash(activeOpt->get().getNickname());
                updateMetricsFromFrame(makeCallMetricsKey(senderHash, "voice"), frameOpt->frameSeq, frameOpt->timestampMs, frameOpt->payloadLen);
            }
            auto decryptedData = m_mediaProcessingService->decryptData(payload, payloadLen, activeOpt->get().getCallKey());
            if (decryptedData.empty()) return;
            auto audioFrame = m_mediaProcessingService->decodeAudioFrame(decryptedData.data(), static_cast<int>(decryptedData.size()));
            if (!audioFrame.empty()) {
                m_audioEngine->playAudio(audioFrame.data(), static_cast<int>(audioFrame.size()));
                updateSpeakingState(activeOpt->get().getNickname(), audioFrame, true);
            }
            sendRttPingIfNeeded();
            sendStatsIfNeeded();
            return;
        }

        if (!m_stateManager->isActiveMeeting()) {
            m_remoteParticipantSpeakingState.clear();
            return;
        }

        auto frameOpt = parseMeetingFrame(data, length);
        if (!frameOpt) return;

        const MeetingFrame& frame = *frameOpt;
        auto meetingOpt = m_stateManager->getActiveMeeting();
        if (frame.meetingId.empty() || !meetingOpt || frame.meetingId != meetingOpt->get().getMeetingId()) return;
        if (frame.mediaKind != 0) return;

        const auto& meetingKey = meetingOpt->get().getMeetingKey();
        if (meetingKey.empty()) return;

        auto decryptedData = m_mediaProcessingService->decryptData(frame.payload, frame.payloadLen, meetingKey);
        if (decryptedData.empty()) return;
        updateMetricsFromFrame(makeStreamMetricsKey(frame.senderHash, "voice", frame.layerId), frame.frameSeq, frame.timestampMs, frame.payloadLen);
        auto audioFrame = m_mediaProcessingService->decodeAudioFrame(decryptedData.data(), static_cast<int>(decryptedData.size()));
        if (!audioFrame.empty()) {
            m_audioEngine->playAudio(audioFrame.data(), static_cast<int>(audioFrame.size()));
        }
        if (!frame.senderHash.empty() && m_eventListener) {
            const std::string nickname = meetingParticipantNicknameByHash(m_stateManager, frame.senderHash);
            if (!nickname.empty()) {
                updateSpeakingState(nickname, audioFrame, false);
            }
        }
        sendRttPingIfNeeded();
        sendStatsIfNeeded();
    }

    void MediaPacketHandler::handleIncomingScreen(const unsigned char* data, int length) {
        if (!m_stateManager->isAuthorized() ||
            m_stateManager->isConnectionDown() ||
            !m_stateManager->isViewingRemoteScreen()) return;

        auto activeOpt = m_stateManager->getActiveCall();
        std::vector<unsigned char> decryptedData;
        if (activeOpt) {
            auto frameOpt = parseMeetingFrame(data, length);
            const unsigned char* payload = data;
            int payloadLen = length;
            if (frameOpt && frameOpt->mediaKind == 1) {
                payload = frameOpt->payload;
                payloadLen = frameOpt->payloadLen;
                const std::string senderHash = core::utilities::crypto::calculateHash(activeOpt->get().getNickname());
                updateMetricsFromFrame(makeCallMetricsKey(senderHash, "screen"), frameOpt->frameSeq, frameOpt->timestampMs, frameOpt->payloadLen);
            }
            decryptedData = m_mediaProcessingService->decryptData(payload, payloadLen, activeOpt->get().getCallKey());
        } else {
            if (!m_stateManager->isActiveMeeting()) return;
            auto frameOpt = parseMeetingFrame(data, length);
            if (!frameOpt) return;
            const MeetingFrame& frame = *frameOpt;
            auto meetingOpt = m_stateManager->getActiveMeeting();
            if (frame.meetingId.empty() || !meetingOpt || frame.meetingId != meetingOpt->get().getMeetingId()) return;
            if (frame.mediaKind != 1) return;
            const auto& meetingKey = meetingOpt->get().getMeetingKey();
            if (meetingKey.empty()) return;
            decryptedData = m_mediaProcessingService->decryptData(frame.payload, frame.payloadLen, meetingKey);
            updateMetricsFromFrame(makeStreamMetricsKey(frame.senderHash, "screen", frame.layerId), frame.frameSeq, frame.timestampMs, frame.payloadLen);
        }
        if (decryptedData.empty()) return;
        const auto videoFrame = m_mediaProcessingService->decodeVideoFrame(MediaType::Screen, decryptedData.data(), static_cast<int>(decryptedData.size()));
        if (!videoFrame.isEmpty() && m_eventListener) {
            m_eventListener->onIncomingScreen(videoFrame);
        }
        sendRttPingIfNeeded();
        sendStatsIfNeeded();
    }

    void MediaPacketHandler::handleIncomingCamera(const unsigned char* data, int length) {
        if (!m_stateManager->isAuthorized() ||
            m_stateManager->isConnectionDown()) return;

        auto activeOpt = m_stateManager->getActiveCall();
        const bool isCallContext = static_cast<bool>(activeOpt);
        if (!isCallContext && !m_stateManager->isViewingAnyRemoteCamera()) {
            return;
        }
        std::vector<unsigned char> decryptedData;
        std::string senderNickname;
        std::string senderStreamKey;
        if (activeOpt) {
            auto frameOpt = parseMeetingFrame(data, length);
            const unsigned char* payload = data;
            int payloadLen = length;
            if (frameOpt && frameOpt->mediaKind == 2) {
                payload = frameOpt->payload;
                payloadLen = frameOpt->payloadLen;
                const std::string senderHash = core::utilities::crypto::calculateHash(activeOpt->get().getNickname());
                updateMetricsFromFrame(makeCallMetricsKey(senderHash, "camera"), frameOpt->frameSeq, frameOpt->timestampMs, frameOpt->payloadLen);
            }
            decryptedData = m_mediaProcessingService->decryptData(payload, payloadLen, activeOpt->get().getCallKey());
            if (!decryptedData.empty()) {
                senderNickname = activeOpt->get().getNickname();
                senderStreamKey = utilities::crypto::calculateHash(senderNickname);
                // Resilient 1:1 behavior: if CAMERA_SHARING_BEGIN was missed/raced,
                // promote remote camera state on first valid media packet.
                if (!m_stateManager->isViewingAnyRemoteCamera()) {
                    m_stateManager->addRemoteCameraSender(senderStreamKey);
                    if (m_eventListener && !senderNickname.empty()) {
                        m_eventListener->onIncomingCameraSharingStarted(senderNickname);
                    }
                }
            }
        } else {
            if (!m_stateManager->isActiveMeeting()) return;
            auto frameOpt = parseMeetingFrame(data, length);
            if (!frameOpt) return;
            const MeetingFrame& frame = *frameOpt;
            auto meetingOpt = m_stateManager->getActiveMeeting();
            if (frame.meetingId.empty() || !meetingOpt || frame.meetingId != meetingOpt->get().getMeetingId()) return;
            if (frame.mediaKind != 2) return;
            const auto& meetingKey = meetingOpt->get().getMeetingKey();
            if (meetingKey.empty()) return;
            decryptedData = m_mediaProcessingService->decryptData(frame.payload, frame.payloadLen, meetingKey);
            if (decryptedData.empty()) return;
            senderNickname = meetingParticipantNicknameByHash(m_stateManager, frame.senderHash);
            senderStreamKey = frame.senderHash;
            updateMetricsFromFrame(makeStreamMetricsKey(frame.senderHash, "camera", frame.layerId), frame.frameSeq, frame.timestampMs, frame.payloadLen);
        }
        if (decryptedData.empty() || senderNickname.empty() || senderStreamKey.empty()) return;
        const auto videoFrame = m_mediaProcessingService->decodeVideoFrame(
            MediaType::Camera,
            senderStreamKey,
            decryptedData.data(),
            static_cast<int>(decryptedData.size()));
        if (!videoFrame.isEmpty() && m_eventListener) {
            m_eventListener->onIncomingCamera(videoFrame, senderNickname);
        }
        sendRttPingIfNeeded();
        sendStatsIfNeeded();
    }

    void MediaPacketHandler::handleAdaptCommand(const nlohmann::json& jsonObject)
    {
        if (!jsonObject.contains(MAX_LAYER)) {
            return;
        }
        const int maxLayer = jsonObject[MAX_LAYER].get<int>();
        using CameraLayer = media::MediaProcessingService::CameraLayer;
        CameraLayer target = CameraLayer::High;
        if (maxLayer <= 0) {
            target = CameraLayer::Low;
        } else if (maxLayer == 1) {
            target = CameraLayer::Mid;
        }
        m_mediaProcessingService->setCameraTargetLayer(target);
    }

    void MediaPacketHandler::handleRttPong(const nlohmann::json& jsonObject)
    {
        if (!jsonObject.contains(PING_ID)) {
            return;
        }
        const uint64_t pingId = jsonObject[PING_ID].get<uint64_t>();
        auto it = m_pendingPings.find(pingId);
        if (it == m_pendingPings.end()) {
            return;
        }
        const auto now = std::chrono::steady_clock::now();
        m_lastRttMs = static_cast<int>(elapsedMs(it->second, now));
        m_pendingPings.erase(it);
    }

    void MediaPacketHandler::updateMetricsFromFrame(const std::string& streamKey, uint32_t frameSeq, uint32_t timestampMs, int payloadLen)
    {
        auto& metrics = m_streamMetrics[streamKey];
        const auto now = std::chrono::steady_clock::now();
        if (!metrics.initialized) {
            metrics.initialized = true;
            metrics.lastSeq = frameSeq;
            metrics.received = 0;
            metrics.lastRemoteTsMs = timestampMs;
            metrics.lastArrival = now;
            metrics.hasTiming = true;
            return;
        }

        // If stream was paused (layer switch/restart), re-baseline sequence to avoid fake loss spikes.
        if (elapsedMs(metrics.lastArrival, now) > 1500) {
            metrics.lastSeq = frameSeq;
            metrics.lastRemoteTsMs = timestampMs;
            metrics.lastArrival = now;
            metrics.warmupFramesLeft = 10;
            metrics.hasTiming = true;
            return;
        }

        metrics.received++;
        if (payloadLen > 0) {
            metrics.bytes += static_cast<uint64_t>(payloadLen);
        }
        if (frameSeq > metrics.lastSeq + 1) {
            if (metrics.warmupFramesLeft <= 0) {
                metrics.lost += (frameSeq - metrics.lastSeq - 1);
            }
        }
        metrics.lastSeq = frameSeq;
        if (metrics.warmupFramesLeft > 0) {
            metrics.warmupFramesLeft--;
        }

        if (metrics.hasTiming) {
            const double arrivalDelta = static_cast<double>(elapsedMs(metrics.lastArrival, now));
            const uint32_t tsDeltaRaw = timestampMs - metrics.lastRemoteTsMs;
            const double transitDiff = std::abs(arrivalDelta - static_cast<double>(tsDeltaRaw));
            metrics.jitterMs += (transitDiff - metrics.jitterMs) / 16.0;
        }
        metrics.lastArrival = now;
        metrics.lastRemoteTsMs = timestampMs;
        metrics.hasTiming = true;
    }

    void MediaPacketHandler::sendRttPingIfNeeded()
    {
        const auto now = std::chrono::steady_clock::now();
        if (!m_sendPacket) {
            return;
        }
        if (m_lastPingSentAt.time_since_epoch().count() != 0
            && now - m_lastPingSentAt < std::chrono::seconds(2)) {
            return;
        }
        m_lastPingSentAt = now;
        const uint64_t pingId = m_nextPingId++;
        m_pendingPings[pingId] = now;
        const uint64_t clientTsMs = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count());
        nlohmann::json ping{
            { SENDER_NICKNAME_HASH, core::utilities::crypto::calculateHash(m_stateManager->getMyNickname()) },
            { PING_ID, pingId },
            { CLIENT_TS_MS, clientTsMs }
        };
        const std::string payload = ping.dump();
        (void)m_sendPacket(std::vector<unsigned char>(payload.begin(), payload.end()), PacketType::MEDIA_RTT_PING);
    }

    void MediaPacketHandler::sendStatsIfNeeded()
    {
        const auto now = std::chrono::steady_clock::now();
        if (!m_sendPacket) {
            return;
        }
        if (m_lastStatsSentAt.time_since_epoch().count() != 0
            && now - m_lastStatsSentAt < std::chrono::seconds(1)) {
            return;
        }
        m_lastStatsSentAt = now;

        constexpr uint64_t kMinSamplesPerStream = 20;
        uint64_t totalRecv = 0;
        uint64_t totalLost = 0;
        uint64_t totalBytes = 0;
        double jitterSum = 0.0;
        size_t jitterCount = 0;
        for (auto& [streamKey, m] : m_streamMetrics) {
            (void)streamKey;
            const uint64_t samples = m.received + m.lost;
            if (samples >= kMinSamplesPerStream && m.warmupFramesLeft <= 0) {
                totalRecv += m.received;
                totalLost += m.lost;
                totalBytes += m.bytes;
            }
            if (m.hasTiming && samples >= kMinSamplesPerStream && m.warmupFramesLeft <= 0) {
                jitterSum += m.jitterMs;
                jitterCount++;
            }
            m.received = 0;
            m.lost = 0;
            m.bytes = 0;
        }
        const double lossPct = (totalRecv + totalLost) == 0 ? 0.0 : (100.0 * static_cast<double>(totalLost) / static_cast<double>(totalRecv + totalLost));
        const int jitterMs = jitterCount == 0 ? 0 : static_cast<int>(std::round(jitterSum / static_cast<double>(jitterCount)));
        const int recvBitrateKbps = static_cast<int>((totalBytes * 8ULL) / 1000ULL);

        nlohmann::json stats{
            { SENDER_NICKNAME_HASH, core::utilities::crypto::calculateHash(m_stateManager->getMyNickname()) },
            { LOSS_PCT, lossPct },
            { JITTER_MS, jitterMs },
            { RTT_MS, m_lastRttMs },
            { RECV_BITRATE_KBPS, recvBitrateKbps }
        };
        const std::string payload = stats.dump();
        (void)m_sendPacket(std::vector<unsigned char>(payload.begin(), payload.end()), PacketType::MEDIA_RECEIVER_STATS);
    }
}