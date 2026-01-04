#include "calliforniaEventListener.h"
#include <QApplication>
#include <QMetaObject>
#include <QMetaType>

#include <vector>
#include "errorCode.h"

#include "../mainWindow.h"

Q_DECLARE_METATYPE(std::vector<unsigned char>)

ClientCallbacksHandler::ClientCallbacksHandler(MainWindow* mainWindow)
    : m_mainWindow(mainWindow)
{
    qRegisterMetaType<std::vector<unsigned char>>("std::vector<unsigned char>");
}

void ClientCallbacksHandler::onAuthorizationResult(std::error_code ec)
{
    if (m_mainWindow) {
        core::ErrorCode errorCode = static_cast<core::ErrorCode>(ec.value());
        QMetaObject::invokeMethod(m_mainWindow, "onAuthorizationResult",
            Qt::QueuedConnection, Q_ARG(core::ErrorCode, errorCode));
    }
}

void ClientCallbacksHandler::onStartOutgoingCallResult(std::error_code ec)
{
    if (m_mainWindow) {
        core::ErrorCode errorCode = static_cast<core::ErrorCode>(ec.value());
        QMetaObject::invokeMethod(m_mainWindow, "onStartCallingResult",
            Qt::QueuedConnection, Q_ARG(core::ErrorCode, errorCode));
    }
}

void ClientCallbacksHandler::onAcceptCallResult(std::error_code ec)
{
    if (m_mainWindow) {
        core::ErrorCode errorCode = static_cast<core::ErrorCode>(ec.value());
        QMetaObject::invokeMethod(m_mainWindow, "onAcceptCallResult",
            Qt::QueuedConnection,
            Q_ARG(core::ErrorCode, errorCode),
            Q_ARG(const QString&, QString()));
    }
}

void ClientCallbacksHandler::onStartScreenSharingResult(std::error_code ec) {
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

void ClientCallbacksHandler::onStopScreenSharingResult(std::error_code ec) {
    // Not used in UI
}

void ClientCallbacksHandler::onIncomingScreenSharingStarted() {
    if (m_mainWindow) {
        QMetaObject::invokeMethod(m_mainWindow, "onIncomingScreenSharingStarted",
            Qt::QueuedConnection);
    }
}

void ClientCallbacksHandler::onIncomingScreenSharingStopped() {
    if (m_mainWindow) {
        QMetaObject::invokeMethod(m_mainWindow, "onIncomingScreenSharingStopped",
            Qt::QueuedConnection);
    }
}

void ClientCallbacksHandler::onIncomingScreen(const std::vector<unsigned char>& data) {
    if (m_mainWindow) {
        QMetaObject::invokeMethod(m_mainWindow, "onIncomingScreen",
            Qt::QueuedConnection, Q_ARG(const std::vector<unsigned char>&, data));
    }
}

void ClientCallbacksHandler::onStartCameraSharingResult(std::error_code ec) {
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

void ClientCallbacksHandler::onStopCameraSharingResult(std::error_code ec) {
    // Not used in UI
}

void ClientCallbacksHandler::onIncomingCameraSharingStarted() {
    if (m_mainWindow) {
        QMetaObject::invokeMethod(m_mainWindow, "onIncomingCameraSharingStarted",
            Qt::QueuedConnection);
    }
}

void ClientCallbacksHandler::onIncomingCameraSharingStopped() {
    if (m_mainWindow) {
        QMetaObject::invokeMethod(m_mainWindow, "onIncomingCameraSharingStopped",
            Qt::QueuedConnection);
    }
}

void ClientCallbacksHandler::onIncomingCamera(const std::vector<unsigned char>& data) {
    if (m_mainWindow) {
        QMetaObject::invokeMethod(m_mainWindow, "onIncomingCamera",
            Qt::QueuedConnection, Q_ARG(const std::vector<unsigned char>&, data));
    }
}

void ClientCallbacksHandler::onOutgoingCallAccepted()
{
    if (m_mainWindow) {
        QMetaObject::invokeMethod(m_mainWindow, "onCallingAccepted",
            Qt::QueuedConnection);
    }
}

void ClientCallbacksHandler::onOutgoingCallDeclined()
{
    if (m_mainWindow) {
        QMetaObject::invokeMethod(m_mainWindow, "onCallingDeclined",
            Qt::QueuedConnection);
    }
}

void ClientCallbacksHandler::onOutgoingCallTimeout(std::error_code ec)
{
    if (m_mainWindow) {
        QMetaObject::invokeMethod(m_mainWindow, "onMaximumCallingTimeReached",
            Qt::QueuedConnection);
    }
}

void ClientCallbacksHandler::onIncomingCall(const std::string& friendNickname)
{
    if (m_mainWindow) {
        QString qFriendNickname = QString::fromStdString(friendNickname);
        QMetaObject::invokeMethod(m_mainWindow, "onIncomingCall",
            Qt::QueuedConnection, Q_ARG(const QString&, qFriendNickname));
    }
}

void ClientCallbacksHandler::onIncomingCallExpired(std::error_code ec, const std::string& friendNickname)
{
    if (m_mainWindow) {
        QString qNickname = QString::fromStdString(friendNickname);
        QMetaObject::invokeMethod(m_mainWindow, "onIncomingCallExpired",
            Qt::QueuedConnection, Q_ARG(const QString&, qNickname));
    }
}

void ClientCallbacksHandler::onConnectionDown()
{
    if (m_mainWindow) {
        QMetaObject::invokeMethod(m_mainWindow, "onClientNetworkError",
            Qt::QueuedConnection);
    }
}

void ClientCallbacksHandler::onConnectionRestored()
{
    if (m_mainWindow) {
        QMetaObject::invokeMethod(m_mainWindow, "onConnectionRestored",
            Qt::QueuedConnection);
    }
}

void ClientCallbacksHandler::onLogoutCompleted()
{
    // Not used in UI
}

void ClientCallbacksHandler::onStopOutgoingCallResult(std::error_code ec)
{
    // Not used in UI
}

void ClientCallbacksHandler::onDeclineCallResult(std::error_code ec)
{
    // Not used in UI
}

void ClientCallbacksHandler::onEndCallResult(std::error_code ec)
{
    // Not used in UI
}

void ClientCallbacksHandler::onCallParticipantConnectionDown()
{
    // Not used in UI
}

void ClientCallbacksHandler::onCallParticipantConnectionRestored()
{
    // Not used in UI
}

void ClientCallbacksHandler::onConnectionRestoredAuthorizationNeeded()
{
    // Not used in UI
}

void ClientCallbacksHandler::onCallEndedByRemote(std::error_code ec)
{
    if (m_mainWindow) {
        QMetaObject::invokeMethod(m_mainWindow, "onRemoteUserEndedCall",
            Qt::QueuedConnection);
    }
}