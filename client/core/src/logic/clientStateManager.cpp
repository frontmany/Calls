#include "clientStateManager.h"

namespace core::logic 
{
    ClientStateManager::ClientStateManager()
        : m_authorized(false)
        , m_connectionDown(false)
        , m_viewingRemoteScreen(false)
        , m_viewingRemoteCamera(false)
    {
        m_mediaState[media::MediaType::Screen] = media::MediaState::Stopped;
        m_mediaState[media::MediaType::Camera] = media::MediaState::Stopped;
        m_mediaState[media::MediaType::Audio] = media::MediaState::Stopped;
    }

    bool ClientStateManager::isOutgoingCall() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_outgoingCall.has_value();
    }

    bool ClientStateManager::isActiveCall() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_activeCall.has_value();
    }

    bool ClientStateManager::isCallParticipantConnectionDown() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (!m_activeCall.has_value()) {
            return false;
        }

        return m_activeCall.value().isParticipantConnectionDown();
    }

    bool ClientStateManager::isConnectionDown() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_connectionDown;
    }

    bool ClientStateManager::isIncomingCalls() const {
        return getIncomingCallsCount() != 0;
    }

    const media::MediaState ClientStateManager::getMediaState(media::MediaType type) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_mediaState.find(type);
        return it == m_mediaState.end() ? media::MediaState::Stopped : it->second;
    }

    bool ClientStateManager::isViewingRemoteScreen() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_viewingRemoteScreen;
    }

    bool ClientStateManager::isViewingRemoteCamera() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_viewingRemoteCamera;
    }

    bool ClientStateManager::isAuthorized() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_authorized;
    }

    void ClientStateManager::setAuthorized(bool value)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_authorized = value;
    }

    void ClientStateManager::setConnectionDown(bool value)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_connectionDown = value;
    }

    void ClientStateManager::setCallParticipantConnectionDown(bool value)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (m_activeCall.has_value()) {
            m_activeCall.value().setParticipantConnectionDown(value);
        }
    }

    void ClientStateManager::setMediaState(media::MediaType type, media::MediaState state) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_mediaState[type] = state;
    }

    void ClientStateManager::setViewingRemoteScreen(bool value)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_viewingRemoteScreen = value;
    }

    void ClientStateManager::setViewingRemoteCamera(bool value)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_viewingRemoteCamera = value;
    }

    const std::string& ClientStateManager::getMyNickname() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_myNickname;
    }

    void ClientStateManager::setMyNickname(const std::string& nickname)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_myNickname = nickname;
    }

    void ClientStateManager::resetMyNickname()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_myNickname.clear();
    }


    const std::string& ClientStateManager::getMyToken() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_myToken;
    }

    const OutgoingCall& ClientStateManager::getOutgoingCall() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (!m_outgoingCall.has_value()) {
            throw std::runtime_error("Outgoing call is not set");
        }

        return m_outgoingCall.value();
    }

    const Call& ClientStateManager::getActiveCall() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (!m_activeCall.has_value()) {
            throw std::runtime_error("Active call is not set");
        }

        return m_activeCall.value();
    }

    void ClientStateManager::setMyToken(const std::string& token)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_myToken = token;
    }

    void ClientStateManager::resetMyToken()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_myToken.clear();
    }

    void ClientStateManager::setActiveCall(const std::string& nickname,
        const CryptoPP::RSA::PublicKey& publicKey, const CryptoPP::SecByteBlock& callKey)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (m_outgoingCall.has_value()) {
            m_outgoingCall->stop();
        }

        m_outgoingCall = std::nullopt;
        m_activeCall.emplace(nickname, publicKey, callKey);
    }

    void ClientStateManager::resetActiveCall() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_activeCall = std::nullopt;
    }

    void ClientStateManager::resetOutgoingCall() {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (m_outgoingCall.has_value()) {
            m_outgoingCall->stop();
        }

        m_outgoingCall = std::nullopt;
    }

    const std::unordered_map<std::string, IncomingCall>& ClientStateManager::getIncomingCalls() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_incomingCalls;
    }

    int ClientStateManager::getIncomingCallsCount() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_incomingCalls.size();
    }

    void ClientStateManager::removeIncomingCall(const std::string& nickname) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_incomingCalls.contains(nickname))
            m_incomingCalls.erase(nickname);
    }

    void ClientStateManager::resetIncomingCalls()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_incomingCalls.clear();
    }

    void ClientStateManager::reset()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_myNickname.clear();
        m_myToken.clear();
        m_incomingCalls.clear();

        if (m_outgoingCall.has_value()) {
            m_outgoingCall->stop();
        }

        m_outgoingCall = std::nullopt;
        m_activeCall = std::nullopt;

        m_authorized = false;
        m_connectionDown = false;
        m_viewingRemoteScreen = false;
        m_viewingRemoteCamera = false;

        m_mediaState[media::MediaType::Screen] = media::MediaState::Stopped;
        m_mediaState[media::MediaType::Camera] = media::MediaState::Stopped;
        m_mediaState[media::MediaType::Audio] = media::MediaState::Stopped;
    }
}