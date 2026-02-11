#include "mediaPacketHandler.h"
#include "logic/clientStateManager.h"
#include "constants/jsonType.h"

using namespace core::constant;

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

        if (utilities::crypto::calculateHash(m_stateManager->getActiveCall().getNickname()) != senderNicknameHash) return;

        m_stateManager->setViewingRemoteScreen(true);
        m_eventListener->onIncomingScreenSharingStarted();
    }

    void MediaPacketHandler::handleIncomingScreenSharingStopped(const nlohmann::json& jsonObject) {
        if (!m_stateManager->isAuthorized() ||
            m_stateManager->isConnectionDown() ||
            !m_stateManager->isActiveCall() ||
            !m_stateManager->isViewingRemoteScreen()) return;

        const std::string& senderNicknameHash = jsonObject[SENDER_NICKNAME_HASH].get<std::string>();

        if (utilities::crypto::calculateHash(m_stateManager->getActiveCall().getNickname()) != senderNicknameHash) return;

        m_stateManager->setViewingRemoteScreen(false);
        m_eventListener->onIncomingScreenSharingStopped();
    }

    void MediaPacketHandler::handleIncomingCameraSharingStarted(const nlohmann::json& jsonObject) {
        if (!m_stateManager->isAuthorized() ||
            m_stateManager->isConnectionDown() ||
            !m_stateManager->isActiveCall() ||
            m_stateManager->isViewingRemoteCamera()) return;
        
        const std::string& senderNicknameHash = jsonObject[SENDER_NICKNAME_HASH].get<std::string>();

        if (utilities::crypto::calculateHash(m_stateManager->getActiveCall().getNickname()) != senderNicknameHash) return;

        m_stateManager->setViewingRemoteCamera(true);
        m_eventListener->onIncomingCameraSharingStarted();
    }

    void MediaPacketHandler::handleIncomingCameraSharingStopped(const nlohmann::json& jsonObject) {
        if (!m_stateManager->isAuthorized() ||
            m_stateManager->isConnectionDown() ||
            !m_stateManager->isActiveCall() ||
            !m_stateManager->isViewingRemoteCamera()) return;

        const std::string& senderNicknameHash = jsonObject[SENDER_NICKNAME_HASH].get<std::string>();

        if (utilities::crypto::calculateHash(m_stateManager->getActiveCall().getNickname()) != senderNicknameHash) return;

        m_stateManager->setViewingRemoteCamera(false);
        m_eventListener->onIncomingCameraSharingStopped();
    }

    void MediaPacketHandler::handleIncomingAudio(const unsigned char* data, int length) {
        if (!m_stateManager->isAuthorized() ||
            m_stateManager->isConnectionDown() ||
            !m_stateManager->isActiveCall() ||
            !m_audioEngine->isStream()) return;
        
        auto decryptedData = m_mediaProcessingService->decryptData(data, length, m_stateManager->getActiveCall().getCallKey());
        if (decryptedData.empty()) return;
        auto audioFrame = m_mediaProcessingService->decodeAudioFrame(decryptedData.data(), static_cast<int>(decryptedData.size()));
        if (!audioFrame.empty()) {
            m_audioEngine->playAudio(audioFrame.data(), static_cast<int>(audioFrame.size()));
        }
    }

    void MediaPacketHandler::handleIncomingScreen(const unsigned char* data, int length) {
        if (!m_stateManager->isAuthorized() ||
            m_stateManager->isConnectionDown() ||
            !m_stateManager->isActiveCall()) return;

        auto decryptedData = m_mediaProcessingService->decryptData(data, length, m_stateManager->getActiveCall().getCallKey());
        if (decryptedData.empty()) return;
        auto videoFrame = m_mediaProcessingService->decodeVideoFrame(decryptedData.data(), static_cast<int>(decryptedData.size()));
        if (!videoFrame.empty() && m_eventListener) {
            m_eventListener->onIncomingScreen(videoFrame);
        }
    }

    void MediaPacketHandler::handleIncomingCamera(const unsigned char* data, int length) {
        if (!m_stateManager->isAuthorized() ||
            m_stateManager->isConnectionDown() ||
            !m_stateManager->isActiveCall()) return;

        auto decryptedData = m_mediaProcessingService->decryptData(data, length, m_stateManager->getActiveCall().getCallKey());
        if (decryptedData.empty()) return;
        auto videoFrame = m_mediaProcessingService->decodeVideoFrame(decryptedData.data(), static_cast<int>(decryptedData.size()));
        if (!videoFrame.empty() && m_eventListener) {
            m_eventListener->onIncomingCamera(videoFrame);
        }
    }
}