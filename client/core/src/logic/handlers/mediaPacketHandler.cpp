#include "mediaPacketHandler.h"
#include "logic/clientStateManager.h"
#include "constants/jsonType.h"
#include "media/mediaType.h"
#include "utilities/crypto.h"

#include <cstdint>

using namespace core::constant;
using namespace core::media;

namespace core::logic 
{
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
            !m_stateManager->isActiveCall() ||
            m_stateManager->isViewingRemoteScreen()) return;

        const std::string& senderNicknameHash = jsonObject[SENDER_NICKNAME_HASH].get<std::string>();

        auto activeOpt = m_stateManager->getActiveCall();
        if (!activeOpt || utilities::crypto::calculateHash(activeOpt->get().getNickname()) != senderNicknameHash) return;

        m_stateManager->setViewingRemoteScreen(true);
        m_eventListener->onIncomingScreenSharingStarted();
    }

    void MediaPacketHandler::handleIncomingScreenSharingStopped(const nlohmann::json& jsonObject) {
        if (!m_stateManager->isAuthorized() ||
            m_stateManager->isConnectionDown() ||
            !m_stateManager->isActiveCall() ||
            !m_stateManager->isViewingRemoteScreen()) return;

        const std::string& senderNicknameHash = jsonObject[SENDER_NICKNAME_HASH].get<std::string>();

        auto activeOpt = m_stateManager->getActiveCall();
        if (!activeOpt || utilities::crypto::calculateHash(activeOpt->get().getNickname()) != senderNicknameHash) return;

        m_stateManager->setViewingRemoteScreen(false);
        m_eventListener->onIncomingScreenSharingStopped();
    }

    void MediaPacketHandler::handleIncomingCameraSharingStarted(const nlohmann::json& jsonObject) {
        if (!m_stateManager->isAuthorized() ||
            m_stateManager->isConnectionDown() ||
            !m_stateManager->isActiveCall() ||
            m_stateManager->isViewingRemoteCamera()) return;
        
        const std::string& senderNicknameHash = jsonObject[SENDER_NICKNAME_HASH].get<std::string>();

        auto activeOpt = m_stateManager->getActiveCall();
        if (!activeOpt || utilities::crypto::calculateHash(activeOpt->get().getNickname()) != senderNicknameHash) return;

        m_stateManager->setViewingRemoteCamera(true);
        m_eventListener->onIncomingCameraSharingStarted();
    }

    void MediaPacketHandler::handleIncomingCameraSharingStopped(const nlohmann::json& jsonObject) {
        if (!m_stateManager->isAuthorized() ||
            m_stateManager->isConnectionDown() ||
            !m_stateManager->isActiveCall() ||
            !m_stateManager->isViewingRemoteCamera()) return;

        const std::string& senderNicknameHash = jsonObject[SENDER_NICKNAME_HASH].get<std::string>();

        auto activeOpt = m_stateManager->getActiveCall();
        if (!activeOpt || utilities::crypto::calculateHash(activeOpt->get().getNickname()) != senderNicknameHash) return;

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

        if (length < 2) return;
        const uint16_t meetingIdLen = (static_cast<uint16_t>(data[0]) << 8) | static_cast<uint16_t>(data[1]);
        if (length < 2 + meetingIdLen) return;

        const std::string meetingId(reinterpret_cast<const char*>(data + 2), meetingIdLen);
        auto meetingOpt = m_stateManager->getActiveMeeting();
        if (meetingId.empty() || !meetingOpt || meetingId != meetingOpt->get().getMeetingId()) return;

        auto binOpt = core::utilities::crypto::hashToBinary(core::utilities::crypto::calculateHash(meetingId));
        if (!binOpt) return;

        CryptoPP::SecByteBlock meetingKey(binOpt->data(), binOpt->size());
        const unsigned char* payload = data + 2 + meetingIdLen;
        const int payloadLen = length - static_cast<int>(2 + meetingIdLen);

        auto decryptedData = m_mediaProcessingService->decryptData(payload, payloadLen, meetingKey);
        if (decryptedData.empty()) return;
        auto audioFrame = m_mediaProcessingService->decodeAudioFrame(decryptedData.data(), static_cast<int>(decryptedData.size()));
        if (!audioFrame.empty()) {
            m_audioEngine->playAudio(audioFrame.data(), static_cast<int>(audioFrame.size()));
        }
    }

    void MediaPacketHandler::handleIncomingScreen(const unsigned char* data, int length) {
        if (!m_stateManager->isAuthorized() ||
            m_stateManager->isConnectionDown() ||
            !m_stateManager->isActiveCall() ||
            !m_stateManager->isViewingRemoteScreen()) return;

        auto activeOpt = m_stateManager->getActiveCall();
        if (!activeOpt) return;
        auto decryptedData = m_mediaProcessingService->decryptData(data, length, activeOpt->get().getCallKey());
        if (decryptedData.empty()) return;
        auto videoFrame = m_mediaProcessingService->decodeVideoFrame(MediaType::Screen, decryptedData.data(), static_cast<int>(decryptedData.size()));
        if (!videoFrame.empty() && m_eventListener) {
            m_eventListener->onIncomingScreen(videoFrame, m_mediaProcessingService->getWidth(MediaType::Screen), m_mediaProcessingService->getHeight(MediaType::Screen));
        }
    }

    void MediaPacketHandler::handleIncomingCamera(const unsigned char* data, int length) {
        if (!m_stateManager->isAuthorized() ||
            m_stateManager->isConnectionDown() ||
            !m_stateManager->isActiveCall() ||
            !m_stateManager->isViewingRemoteCamera()) return;

        auto activeOpt = m_stateManager->getActiveCall();
        if (!activeOpt) return;
        auto decryptedData = m_mediaProcessingService->decryptData(data, length, activeOpt->get().getCallKey());
        if (decryptedData.empty()) return;
        auto videoFrame = m_mediaProcessingService->decodeVideoFrame(MediaType::Camera, decryptedData.data(), static_cast<int>(decryptedData.size()));
        if (!videoFrame.empty() && m_eventListener) {
            m_eventListener->onIncomingCamera(videoFrame, m_mediaProcessingService->getWidth(MediaType::Camera), m_mediaProcessingService->getHeight(MediaType::Camera));
        }
    }
}