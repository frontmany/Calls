#include "clientStateManager.h"

using namespace core;

ClientStateManager::ClientStateManager()
    : m_connectionDown(false),
    m_screenSharing(false),
    m_viewingRemoteScreen(false),
    m_cameraSharing(false),
    m_viewingRemoteCamera(false)
{
}

bool ClientStateManager::isOutgoingCall() const
{
    return m_callStateManager.isOutgoingCall();
}

bool ClientStateManager::isActiveCall() const
{
    return m_callStateManager.isActiveCall();
}

bool ClientStateManager::isCallParticipantConnectionLost() const
{
    return m_callStateManager.isCallParticipantConnectionDown();
}

bool ClientStateManager::isConnectionDown() const
{
    return m_connectionDown.load();
}

bool ClientStateManager::isIncomingCalls() const {
    return getIncomingCallsCount() != 0;
}

bool ClientStateManager::isScreenSharing() const
{
    return m_screenSharing.load();
}

bool ClientStateManager::isViewingRemoteScreen() const
{
    return m_viewingRemoteScreen.load();
}

bool ClientStateManager::isCameraSharing() const
{
    return m_cameraSharing.load();
}

bool ClientStateManager::isViewingRemoteCamera() const
{
    return m_viewingRemoteCamera.load();
}

bool ClientStateManager::isAuthorized() const
{
    return m_authorized.load();
}

void ClientStateManager::setAuthorized(bool value)
{
    m_authorized.store(value);
}

void ClientStateManager::setConnectionDown(bool value)
{
    m_connectionDown.store(value);
}

void ClientStateManager::setCallParticipantConnectionDown(bool value)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_callStateManager.setCallParticipantConnectionDown(value);
}

void ClientStateManager::setScreenSharing(bool value)
{
    m_screenSharing.store(value);
}

void ClientStateManager::setViewingRemoteScreen(bool value)
{
    m_viewingRemoteScreen.store(value);
}

void ClientStateManager::setCameraSharing(bool value)
{
    m_cameraSharing.store(value);
}

void ClientStateManager::setViewingRemoteCamera(bool value)
{
    m_viewingRemoteCamera.store(value);
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

void ClientStateManager::clearMyNickname()
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
    return m_callStateManager.getOutgoingCall();
}

const Call& ClientStateManager::getActiveCall() const
{
    return m_callStateManager.getActiveCall();
}

void ClientStateManager::setMyToken(const std::string& token) 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_myToken = token;
}

void ClientStateManager::clearMyToken() 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_myToken.clear();
}

void ClientStateManager::setActiveCall(const std::string& nickname,
    const CryptoPP::RSA::PublicKey& publicKey, const CryptoPP::SecByteBlock& callKey)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_callStateManager.setActiveCall(nickname, publicKey, callKey);
}

void ClientStateManager::clearCallState()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_callStateManager.clear();
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

void ClientStateManager::clearIncomingCalls()
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
    m_callStateManager.clear();

    m_authorized.store(false);
    m_connectionDown.store(false);
    m_screenSharing.store(false);
    m_viewingRemoteScreen.store(false);
    m_cameraSharing.store(false);
    m_viewingRemoteCamera.store(false);
}