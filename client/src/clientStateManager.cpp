#include "clientStateManager.h"

#include "jsonTypes.h"
#include "json.hpp"

using namespace calls;

ClientStateManager::ClientStateManager()
    : m_connectionDown(false),
    m_screenSharing(false),
    m_viewingRemoteScreen(false),
    m_cameraSharing(false),
    m_viewingRemoteCamera(false)
{
}

bool ClientStateManager::isConnectionDown() const
{
    return m_connectionDown.load();
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
    return m_authorized;
}

void ClientStateManager::setConnectionDown(bool value)
{
    m_connectionDown.store(value);
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
    return m_myNickname;
}

void ClientStateManager::setMyNickname(const std::string& nickname)
{
    m_myNickname = nickname;
}

void ClientStateManager::clearMyNickname()
{
    m_myNickname.clear();
}

const CallStateManager& ClientStateManager::getCallStateManager() const
{
    return m_callStateManager;
}

void ClientStateManager::setActiveCall(const std::string& nickname,
    const CryptoPP::RSA::PublicKey& publicKey)
{
    m_callStateManager.setActiveCall(nickname, publicKey);
}

void ClientStateManager::setActiveCall(const IncomingCall& incomingCall)
{
    m_callStateManager.setActiveCall(incomingCall);
}

void ClientStateManager::clearCallState()
{
    m_callStateManager.clear();
}

const std::unordered_map<std::string, IncomingCall>& ClientStateManager::getIncomingCalls() const
{
    return m_incomingCalls;
}

void ClientStateManager::removeIncomingCall(const std::string& nickname) {
    if (m_incomingCalls.contains(nickname))
        m_incomingCalls.erase(nickname);
}

void ClientStateManager::clearIncomingCalls()
{
    m_incomingCalls.clear();
}

void ClientStateManager::reset()
{
    m_myNickname.clear();
    m_callStateManager.clear();
    m_incomingCalls.clear();

    m_screenSharing = false;
    m_viewingRemoteScreen = false;
    m_cameraSharing = false;
    m_viewingRemoteCamera = false;
}