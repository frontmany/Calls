#include "mediaPacketHandler.h"
#include "logic/clientStateManager.h"
#include "constants/jsonType.h"
#include "media/mediaType.h"
#include "utilities/crypto.h"

#include <cstdint>
#include <optional>

using namespace core::constant;
using namespace core::media;

namespace core::logic
{
    namespace
    {
        struct MeetingFrame {
            std::string meetingId;
            std::string senderHash;
            const unsigned char* payload = nullptr;
            int payloadLen = 0;
        };

        std::optional<MeetingFrame> parseMeetingFrame(const unsigned char* data, int length) {
            if (length < 2) return std::nullopt;
            const uint16_t meetingIdLen = (static_cast<uint16_t>(data[0]) << 8) | static_cast<uint16_t>(data[1]);
            if (length < 2 + meetingIdLen + 2) return std::nullopt;
            const std::string meetingId(reinterpret_cast<const char*>(data + 2), meetingIdLen);
            const uint16_t senderHashLen = (static_cast<uint16_t>(data[2 + meetingIdLen]) << 8) | static_cast<uint16_t>(data[2 + meetingIdLen + 1]);
            if (length < 2 + meetingIdLen + 2 + senderHashLen) return std::nullopt;
            const std::string senderHash(reinterpret_cast<const char*>(data + 2 + meetingIdLen + 2), senderHashLen);
            const unsigned char* payload = data + 2 + meetingIdLen + 2 + senderHashLen;
            const int payloadLen = length - static_cast<int>(2 + meetingIdLen + 2 + senderHashLen);
            MeetingFrame frame;
            frame.meetingId = meetingId;
            frame.senderHash = senderHash;
            frame.payload = payload;
            frame.payloadLen = payloadLen;
            return frame;
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
        std::shared_ptr<EventListener> eventListener)
        : m_stateManager(stateManager)
        , m_audioEngine(audioEngine)
        , m_mediaProcessingService(mediaProcessingService)
        , m_eventListener(eventListener)
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
            m_stateManager->isConnectionDown() ||
            m_stateManager->isViewingRemoteCamera()) return;
        
        const std::string& senderNicknameHash = jsonObject[SENDER_NICKNAME_HASH].get<std::string>();

        auto activeOpt = m_stateManager->getActiveCall();
        if (activeOpt) {
            if (utilities::crypto::calculateHash(activeOpt->get().getNickname()) != senderNicknameHash) return;
        } else {
            if (!m_stateManager->isActiveMeeting() || !isMeetingParticipantHash(m_stateManager, senderNicknameHash)) return;
        }

        m_stateManager->setViewingRemoteCamera(true);
        m_eventListener->onIncomingCameraSharingStarted();
    }

    void MediaPacketHandler::handleIncomingCameraSharingStopped(const nlohmann::json& jsonObject) {
        if (!m_stateManager->isAuthorized() ||
            m_stateManager->isConnectionDown() ||
            !m_stateManager->isViewingRemoteCamera()) return;

        const std::string& senderNicknameHash = jsonObject[SENDER_NICKNAME_HASH].get<std::string>();

        auto activeOpt = m_stateManager->getActiveCall();
        if (activeOpt) {
            if (utilities::crypto::calculateHash(activeOpt->get().getNickname()) != senderNicknameHash) return;
        } else {
            if (!m_stateManager->isActiveMeeting() || !isMeetingParticipantHash(m_stateManager, senderNicknameHash)) return;
        }

        m_stateManager->setViewingRemoteCamera(false);
        m_eventListener->onIncomingCameraSharingStopped();
    }

    void MediaPacketHandler::handleIncomingAudio(const unsigned char* data, int length) {
        if (!m_stateManager->isAuthorized() ||
            m_stateManager->isConnectionDown() ||
            !m_audioEngine->isStream()) return;

        auto activeOpt = m_stateManager->getActiveCall();
        if (activeOpt) {
            auto decryptedData = m_mediaProcessingService->decryptData(data, length, activeOpt->get().getCallKey());
            if (decryptedData.empty()) return;
            auto audioFrame = m_mediaProcessingService->decodeAudioFrame(decryptedData.data(), static_cast<int>(decryptedData.size()));
            if (!audioFrame.empty()) {
                m_audioEngine->playAudio(audioFrame.data(), static_cast<int>(audioFrame.size()));
            }
            return;
        }

        if (!m_stateManager->isActiveMeeting()) {
            return;
        }

        auto frameOpt = parseMeetingFrame(data, length);
        if (!frameOpt) return;

        const MeetingFrame& frame = *frameOpt;
        auto meetingOpt = m_stateManager->getActiveMeeting();
        if (frame.meetingId.empty() || !meetingOpt || frame.meetingId != meetingOpt->get().getMeetingId()) return;

        auto binOpt = core::utilities::crypto::hashToBinary(core::utilities::crypto::calculateHash(frame.meetingId));
        if (!binOpt) return;

        CryptoPP::SecByteBlock meetingKey(binOpt->data(), binOpt->size());

        auto decryptedData = m_mediaProcessingService->decryptData(frame.payload, frame.payloadLen, meetingKey);
        if (decryptedData.empty()) return;
        auto audioFrame = m_mediaProcessingService->decodeAudioFrame(decryptedData.data(), static_cast<int>(decryptedData.size()));
        if (!audioFrame.empty()) {
            m_audioEngine->playAudio(audioFrame.data(), static_cast<int>(audioFrame.size()));
        }
        if (!frame.senderHash.empty() && m_eventListener) {
            const std::string nickname = meetingParticipantNicknameByHash(m_stateManager, frame.senderHash);
            if (!nickname.empty()) {
                m_eventListener->onMeetingParticipantSpeaking(nickname, true);
            }
        }
    }

    void MediaPacketHandler::handleIncomingScreen(const unsigned char* data, int length) {
        if (!m_stateManager->isAuthorized() ||
            m_stateManager->isConnectionDown() ||
            !m_stateManager->isViewingRemoteScreen()) return;

        auto activeOpt = m_stateManager->getActiveCall();
        std::vector<unsigned char> decryptedData;
        if (activeOpt) {
            decryptedData = m_mediaProcessingService->decryptData(data, length, activeOpt->get().getCallKey());
        } else {
            if (!m_stateManager->isActiveMeeting()) return;
            auto frameOpt = parseMeetingFrame(data, length);
            if (!frameOpt) return;
            const MeetingFrame& frame = *frameOpt;
            auto meetingOpt = m_stateManager->getActiveMeeting();
            if (frame.meetingId.empty() || !meetingOpt || frame.meetingId != meetingOpt->get().getMeetingId()) return;
            auto binOpt = core::utilities::crypto::hashToBinary(core::utilities::crypto::calculateHash(frame.meetingId));
            if (!binOpt) return;
            CryptoPP::SecByteBlock meetingKey(binOpt->data(), binOpt->size());
            decryptedData = m_mediaProcessingService->decryptData(frame.payload, frame.payloadLen, meetingKey);
        }
        if (decryptedData.empty()) return;
        auto videoFrame = m_mediaProcessingService->decodeVideoFrame(MediaType::Screen, decryptedData.data(), static_cast<int>(decryptedData.size()));
        if (!videoFrame.empty() && m_eventListener) {
            m_eventListener->onIncomingScreen(videoFrame, m_mediaProcessingService->getWidth(MediaType::Screen), m_mediaProcessingService->getHeight(MediaType::Screen));
        }
    }

    void MediaPacketHandler::handleIncomingCamera(const unsigned char* data, int length) {
        if (!m_stateManager->isAuthorized() ||
            m_stateManager->isConnectionDown() ||
            !m_stateManager->isViewingRemoteCamera()) return;

        auto activeOpt = m_stateManager->getActiveCall();
        std::vector<unsigned char> decryptedData;
        std::string senderNickname;
        if (activeOpt) {
            decryptedData = m_mediaProcessingService->decryptData(data, length, activeOpt->get().getCallKey());
            if (!decryptedData.empty()) {
                senderNickname = activeOpt->get().getNickname();
            }
        } else {
            if (!m_stateManager->isActiveMeeting()) return;
            auto frameOpt = parseMeetingFrame(data, length);
            if (!frameOpt) return;
            const MeetingFrame& frame = *frameOpt;
            auto meetingOpt = m_stateManager->getActiveMeeting();
            if (frame.meetingId.empty() || !meetingOpt || frame.meetingId != meetingOpt->get().getMeetingId()) return;
            auto binOpt = core::utilities::crypto::hashToBinary(core::utilities::crypto::calculateHash(frame.meetingId));
            if (!binOpt) return;
            CryptoPP::SecByteBlock meetingKey(binOpt->data(), binOpt->size());
            decryptedData = m_mediaProcessingService->decryptData(frame.payload, frame.payloadLen, meetingKey);
            if (decryptedData.empty()) return;
            senderNickname = meetingParticipantNicknameByHash(m_stateManager, frame.senderHash);
        }
        if (decryptedData.empty() || senderNickname.empty()) return;
        auto videoFrame = m_mediaProcessingService->decodeVideoFrame(MediaType::Camera, decryptedData.data(), static_cast<int>(decryptedData.size()));
        if (!videoFrame.empty() && m_eventListener) {
            m_eventListener->onIncomingCamera(videoFrame,
                m_mediaProcessingService->getWidth(MediaType::Camera),
                m_mediaProcessingService->getHeight(MediaType::Camera),
                senderNickname);
        }
    }
}