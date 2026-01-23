#include "coreEventListener.h"
#include <QApplication>
#include <vector>

#include "../managers/authorizationManager.h"
#include "../managers/callManager.h"
#include "../media/screenSharingManager.h"
#include "../media/cameraSharingManager.h"
#include "../managers/coreNetworkErrorHandler.h"

CoreEventListener::CoreEventListener(AuthorizationManager* authorizationManager, CallManager* callManager, ScreenSharingManager* screenSharingManager, CameraSharingManager* cameraSharingManager, CoreNetworkErrorHandler* networkErrorHandler)
    : m_authorizationManager(authorizationManager)
    , m_callManager(callManager)
    , m_screenSharingManager(screenSharingManager)
    , m_cameraSharingManager(cameraSharingManager)
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

void CoreEventListener::onStartOutgoingCallResult(std::error_code ec)
{
    if (m_callManager) {
        QMetaObject::invokeMethod(m_callManager, "onStartCallingResult",
            Qt::QueuedConnection, Q_ARG(std::error_code, ec));
    }
}

void CoreEventListener::onAcceptCallResult(std::error_code ec, const std::string& nickname)
{
    if (m_callManager) {
        QMetaObject::invokeMethod(m_callManager, "onAcceptCallResult",
            Qt::QueuedConnection,
            Q_ARG(std::error_code, ec),
            Q_ARG(const QString&, QString::fromStdString(nickname)));
    }
}

void CoreEventListener::onStartScreenSharingResult(std::error_code ec) {
    if (m_screenSharingManager) {
        if (ec) {
            QMetaObject::invokeMethod(m_screenSharingManager, "onStartScreenSharingError",
                Qt::QueuedConnection);
        } else {
            QMetaObject::invokeMethod(m_screenSharingManager, "onScreenSharingStarted",
                Qt::QueuedConnection);
        }
    }
}

void CoreEventListener::onStopScreenSharingResult(std::error_code ec) {
    if (m_screenSharingManager) {
        QMetaObject::invokeMethod(m_screenSharingManager, "onStopScreenSharingResult",
            Qt::QueuedConnection, Q_ARG(std::error_code, ec));
    }
}

void CoreEventListener::onIncomingScreenSharingStarted() {
    if (m_screenSharingManager) {
        QMetaObject::invokeMethod(m_screenSharingManager, "onIncomingScreenSharingStarted",
            Qt::QueuedConnection);
    }
}

void CoreEventListener::onIncomingScreenSharingStopped() {
    if (m_screenSharingManager) {
        QMetaObject::invokeMethod(m_screenSharingManager, "onIncomingScreenSharingStopped",
            Qt::QueuedConnection);
    }
}

void CoreEventListener::onIncomingScreen(const std::vector<unsigned char>& data) {
    if (m_screenSharingManager) {
        QMetaObject::invokeMethod(m_screenSharingManager, "onIncomingScreen",
            Qt::QueuedConnection, Q_ARG(const std::vector<unsigned char>&, data));
    }
}

void CoreEventListener::onStartCameraSharingResult(std::error_code ec) {
    if (m_cameraSharingManager) {
        if (ec) {
            QMetaObject::invokeMethod(m_cameraSharingManager, "onStartCameraSharingError",
                Qt::QueuedConnection);
        } else {
            QMetaObject::invokeMethod(m_cameraSharingManager, "onCameraSharingStarted",
                Qt::QueuedConnection);
        }
    }
}

void CoreEventListener::onStopCameraSharingResult(std::error_code ec) {
    if (m_cameraSharingManager) {
        QMetaObject::invokeMethod(m_cameraSharingManager, "onStopCameraSharingResult",
            Qt::QueuedConnection, Q_ARG(std::error_code, ec));
    }
}

void CoreEventListener::onIncomingCameraSharingStarted() {
    if (m_cameraSharingManager) {
        QMetaObject::invokeMethod(m_cameraSharingManager, "onIncomingCameraSharingStarted",
            Qt::QueuedConnection);
    }
}

void CoreEventListener::onIncomingCameraSharingStopped() {
    if (m_cameraSharingManager) {
        QMetaObject::invokeMethod(m_cameraSharingManager, "onIncomingCameraSharingStopped",
            Qt::QueuedConnection);
    }
}

void CoreEventListener::onIncomingCamera(const std::vector<unsigned char>& data) {
    if (m_cameraSharingManager) {
        QMetaObject::invokeMethod(m_cameraSharingManager, "onIncomingCamera",
            Qt::QueuedConnection, Q_ARG(const std::vector<unsigned char>&, data));
    }
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

void CoreEventListener::onLogoutCompleted()
{
    if (m_authorizationManager) {
        QMetaObject::invokeMethod(m_authorizationManager, "onLogoutCompleted",
            Qt::QueuedConnection);
    }
}

void CoreEventListener::onStopOutgoingCallResult(std::error_code ec)
{
    if (m_callManager) {
        QMetaObject::invokeMethod(m_callManager, "onStopOutgoingCallResult",
            Qt::QueuedConnection, Q_ARG(std::error_code, ec));
    }
}

void CoreEventListener::onDeclineCallResult(std::error_code ec, const std::string& nickname)
{
    if (m_callManager) {
        QString qNickname = QString::fromStdString(nickname);
        QMetaObject::invokeMethod(m_callManager, "onDeclineCallResult",
            Qt::QueuedConnection,
            Q_ARG(std::error_code, ec),
            Q_ARG(const QString&, qNickname));
    }
}

void CoreEventListener::onEndCallResult(std::error_code ec)
{
    if (m_callManager) {
        QMetaObject::invokeMethod(m_callManager, "onEndCallResult",
            Qt::QueuedConnection, Q_ARG(std::error_code, ec));
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
