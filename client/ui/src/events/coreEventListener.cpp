#include "coreEventListener.h"
#include <QApplication>
#include <vector>

#include "errorCode.h"
#include "../mainWindow.h"

CoreEventListener::CoreEventListener(MainWindow* mainWindow)
    : m_mainWindow(mainWindow)
{
}

void CoreEventListener::onAuthorizationResult(std::error_code ec)
{
    if (m_mainWindow) {
        QMetaObject::invokeMethod(m_mainWindow, "onAuthorizationResult",
            Qt::QueuedConnection, Q_ARG(std::error_code, ec));
    }
}

void CoreEventListener::onStartOutgoingCallResult(std::error_code ec)
{
    if (m_mainWindow) {
        QMetaObject::invokeMethod(m_mainWindow, "onStartCallingResult",
            Qt::QueuedConnection, Q_ARG(std::error_code, ec));
    }
}

void CoreEventListener::onAcceptCallResult(std::error_code ec)
{
    if (m_mainWindow) {
        QMetaObject::invokeMethod(m_mainWindow, "onAcceptCallResult",
            Qt::QueuedConnection,
            Q_ARG(std::error_code, ec),
            Q_ARG(const QString&, QString()));
    }
}

void CoreEventListener::onStartScreenSharingResult(std::error_code ec) {
    if (m_mainWindow) {
        if (ec) {
            QMetaObject::invokeMethod(m_mainWindow, "onStartScreenSharingError",
                Qt::QueuedConnection);
        } else {
            QMetaObject::invokeMethod(m_mainWindow, "onScreenSharingStarted",
                Qt::QueuedConnection);
        }
    }
}

void CoreEventListener::onStopScreenSharingResult(std::error_code ec) {
    // Not used in UI
}

void CoreEventListener::onIncomingScreenSharingStarted() {
    if (m_mainWindow) {
        QMetaObject::invokeMethod(m_mainWindow, "onIncomingScreenSharingStarted",
            Qt::QueuedConnection);
    }
}

void CoreEventListener::onIncomingScreenSharingStopped() {
    if (m_mainWindow) {
        QMetaObject::invokeMethod(m_mainWindow, "onIncomingScreenSharingStopped",
            Qt::QueuedConnection);
    }
}

void CoreEventListener::onIncomingScreen(const std::vector<unsigned char>& data) {
    if (m_mainWindow) {
        QMetaObject::invokeMethod(m_mainWindow, "onIncomingScreen",
            Qt::QueuedConnection, Q_ARG(const std::vector<unsigned char>&, data));
    }
}

void CoreEventListener::onStartCameraSharingResult(std::error_code ec) {
    if (m_mainWindow) {
        if (ec) {
            QMetaObject::invokeMethod(m_mainWindow, "onStartCameraSharingError",
                Qt::QueuedConnection);
        } else {
            QMetaObject::invokeMethod(m_mainWindow, "onCameraSharingStarted",
                Qt::QueuedConnection);
        }
    }
}

void CoreEventListener::onStopCameraSharingResult(std::error_code ec) {
    // Not used in UI
}

void CoreEventListener::onIncomingCameraSharingStarted() {
    if (m_mainWindow) {
        QMetaObject::invokeMethod(m_mainWindow, "onIncomingCameraSharingStarted",
            Qt::QueuedConnection);
    }
}

void CoreEventListener::onIncomingCameraSharingStopped() {
    if (m_mainWindow) {
        QMetaObject::invokeMethod(m_mainWindow, "onIncomingCameraSharingStopped",
            Qt::QueuedConnection);
    }
}

void CoreEventListener::onIncomingCamera(const std::vector<unsigned char>& data) {
    if (m_mainWindow) {
        QMetaObject::invokeMethod(m_mainWindow, "onIncomingCamera",
            Qt::QueuedConnection, Q_ARG(const std::vector<unsigned char>&, data));
    }
}

void CoreEventListener::onOutgoingCallAccepted()
{
    if (m_mainWindow) {
        QMetaObject::invokeMethod(m_mainWindow, "onCallingAccepted",
            Qt::QueuedConnection);
    }
}

void CoreEventListener::onOutgoingCallDeclined()
{
    if (m_mainWindow) {
        QMetaObject::invokeMethod(m_mainWindow, "onCallingDeclined",
            Qt::QueuedConnection);
    }
}

void CoreEventListener::onOutgoingCallTimeout(std::error_code ec)
{
    if (m_mainWindow) {
        QMetaObject::invokeMethod(m_mainWindow, "onMaximumCallingTimeReached",
            Qt::QueuedConnection);
    }
}

void CoreEventListener::onIncomingCall(const std::string& friendNickname)
{
    if (m_mainWindow) {
        QString qFriendNickname = QString::fromStdString(friendNickname);
        QMetaObject::invokeMethod(m_mainWindow, "onIncomingCall",
            Qt::QueuedConnection, Q_ARG(const QString&, qFriendNickname));
    }
}

void CoreEventListener::onIncomingCallExpired(std::error_code ec, const std::string& friendNickname)
{
    if (m_mainWindow) {
        QString qNickname = QString::fromStdString(friendNickname);
        QMetaObject::invokeMethod(m_mainWindow, "onIncomingCallExpired",
            Qt::QueuedConnection, Q_ARG(const QString&, qNickname));
    }
}

void CoreEventListener::onConnectionDown()
{
    if (m_mainWindow) {
        QMetaObject::invokeMethod(m_mainWindow, "onClientNetworkError",
            Qt::QueuedConnection);
    }
}

void CoreEventListener::onConnectionRestored()
{
    if (m_mainWindow) {
        QMetaObject::invokeMethod(m_mainWindow, "onConnectionRestored",
            Qt::QueuedConnection);
    }
}

void CoreEventListener::onLogoutCompleted()
{
    // Not used in UI
}

void CoreEventListener::onStopOutgoingCallResult(std::error_code ec)
{
    // Not used in UI
}

void CoreEventListener::onDeclineCallResult(std::error_code ec)
{
    // Not used in UI
}

void CoreEventListener::onEndCallResult(std::error_code ec)
{
    // Not used in UI
}

void CoreEventListener::onCallParticipantConnectionDown()
{
    // Not used in UI
}

void CoreEventListener::onCallParticipantConnectionRestored()
{
    // Not used in UI
}

void CoreEventListener::onConnectionRestoredAuthorizationNeeded()
{
    // Not used in UI
}

void CoreEventListener::onCallEndedByRemote(std::error_code ec)
{
    if (m_mainWindow) {
        QMetaObject::invokeMethod(m_mainWindow, "onRemoteUserEndedCall",
            Qt::QueuedConnection);
    }
}