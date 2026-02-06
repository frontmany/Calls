#include "coreEventListener.h"
#include <QApplication>
#include <vector>

#include "../managers/authorizationManager.h"
#include "../managers/callManager.h"
#include "../managers/coreNetworkErrorHandler.h"

CoreEventListener::CoreEventListener(AuthorizationManager* authorizationManager, CallManager* callManager, CoreNetworkErrorHandler* networkErrorHandler)
    : m_authorizationManager(authorizationManager)
    , m_callManager(callManager)
    , m_coreNetworkErrorHandler(networkErrorHandler)
{
}

void CoreEventListener::onAuthorizationResult(std::error_code ec)
{
    if (m_authorizationManager) {
        QMetaObject::invokeMethod(m_authorizationManager, "onAuthorizationResult",
            Qt::QueuedConnection, Q_ARG(std::error_code, ec));
    }
}

void CoreEventListener::onIncomingScreenSharingStarted() {
    // Теперь обрабатывается в core
}

void CoreEventListener::onIncomingScreenSharingStopped() {
    // Теперь обрабатывается в core
}

void CoreEventListener::onIncomingScreen(const std::vector<unsigned char>& data) {
    // Теперь обрабатывается в core
}

void CoreEventListener::onScreenSharingStarted() {
    // Теперь обрабатывается в core
}

void CoreEventListener::onScreenSharingStopped() {
    // Теперь обрабатывается в core
}

void CoreEventListener::onStartScreenSharingError() {
    // Теперь обрабатывается в core
}

void CoreEventListener::onIncomingCameraSharingStarted() {
    // Теперь обрабатывается в core
}

void CoreEventListener::onIncomingCameraSharingStopped() {
    // Теперь обрабатывается в core
}

void CoreEventListener::onIncomingCamera(const std::vector<unsigned char>& data) {
    // Теперь обрабатывается в core
}

void CoreEventListener::onCameraSharingStarted() {
    // Теперь обрабатывается в core
}

void CoreEventListener::onCameraSharingStopped() {
    // Теперь обрабатывается в core
}

void CoreEventListener::onStartCameraSharingError() {
    // Теперь обрабатывается в core
}

void CoreEventListener::onOutgoingCallAccepted()
{
    if (m_callManager) {
        QMetaObject::invokeMethod(m_callManager, "onCallingAccepted",
            Qt::QueuedConnection);
    }
}

void CoreEventListener::onOutgoingCallDeclined()
{
    if (m_callManager) {
        QMetaObject::invokeMethod(m_callManager, "onCallingDeclined",
            Qt::QueuedConnection);
    }
}

void CoreEventListener::onOutgoingCallTimeout(std::error_code ec)
{
    if (m_callManager) {
        QMetaObject::invokeMethod(m_callManager, "onMaximumCallingTimeReached",
            Qt::QueuedConnection);
    }
}

void CoreEventListener::onIncomingCall(const std::string& friendNickname)
{
    if (m_callManager) {
        QString qFriendNickname = QString::fromStdString(friendNickname);
        QMetaObject::invokeMethod(m_callManager, "onIncomingCall",
            Qt::QueuedConnection, Q_ARG(const QString&, qFriendNickname));
    }
}

void CoreEventListener::onIncomingCallExpired(std::error_code ec, const std::string& friendNickname)
{
    if (m_callManager) {
        QString qNickname = QString::fromStdString(friendNickname);
        QMetaObject::invokeMethod(m_callManager, "onIncomingCallExpired",
            Qt::QueuedConnection, Q_ARG(const QString&, qNickname));
    }
}

void CoreEventListener::onCallParticipantConnectionDown()
{
    if (m_callManager) {
        QMetaObject::invokeMethod(m_callManager, "onCallParticipantConnectionDown",
            Qt::QueuedConnection);
    }
}

void CoreEventListener::onCallParticipantConnectionRestored()
{
    if (m_callManager) {
        QMetaObject::invokeMethod(m_callManager, "onCallParticipantConnectionRestored",
            Qt::QueuedConnection);
    }
}

void CoreEventListener::onConnectionDown()
{
    if (m_coreNetworkErrorHandler) {
        QMetaObject::invokeMethod(static_cast<QObject*>(m_coreNetworkErrorHandler), "onConnectionDown",
            Qt::QueuedConnection);
    }
}

void CoreEventListener::onConnectionRestored()
{
    if (m_coreNetworkErrorHandler) {
        QMetaObject::invokeMethod(static_cast<QObject*>(m_coreNetworkErrorHandler), "onConnectionRestored",
            Qt::QueuedConnection);
    }
}

void CoreEventListener::onConnectionRestoredAuthorizationNeeded()
{
    if (m_coreNetworkErrorHandler) {
        QMetaObject::invokeMethod(static_cast<QObject*>(m_coreNetworkErrorHandler), "onConnectionRestoredAuthorizationNeeded",
            Qt::QueuedConnection);
    }
}

void CoreEventListener::onCallEndedByRemote(std::error_code ec)
{
    if (m_callManager) {
        QMetaObject::invokeMethod(m_callManager, "onRemoteUserEndedCall",
            Qt::QueuedConnection);
    }
}
