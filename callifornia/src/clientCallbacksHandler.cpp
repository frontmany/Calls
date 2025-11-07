#include "clientCallbacksHandler.h"
#include <QMetaObject>
#include <QApplication>
#include "mainWindow.h"

ClientCallbacksHandler::ClientCallbacksHandler(MainWindow* mainWindow)
    : m_mainWindow(mainWindow)
{
}

void ClientCallbacksHandler::onAuthorizationResult(calls::ErrorCode ec)
{
    if (m_mainWindow) {
        QMetaObject::invokeMethod(m_mainWindow, "onAuthorizationResult",
            Qt::QueuedConnection, Q_ARG(calls::ErrorCode, ec));
    }
}

void ClientCallbacksHandler::onStartCallingResult(calls::ErrorCode ec)
{
    if (m_mainWindow) {
        QMetaObject::invokeMethod(m_mainWindow, "onStartCallingResult",
            Qt::QueuedConnection, Q_ARG(calls::ErrorCode, ec));
    }
}

void ClientCallbacksHandler::onAcceptCallResult(calls::ErrorCode ec, const std::string& nickname)
{
    if (m_mainWindow) {
        QString qNickname = QString::fromStdString(nickname);
        QMetaObject::invokeMethod(m_mainWindow, "onAcceptCallResult",
            Qt::QueuedConnection,
            Q_ARG(calls::ErrorCode, ec),
            Q_ARG(const QString&, qNickname));
    }
}

void ClientCallbacksHandler::onStartScreenSharingError() {
    if (m_mainWindow) {
        QMetaObject::invokeMethod(m_mainWindow, "onStartScreenSharingError",
            Qt::QueuedConnection);
    }
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

void ClientCallbacksHandler::onIncomingScreen(const std::string& data) {
    if (m_mainWindow) {
        QMetaObject::invokeMethod(m_mainWindow, "onIncomingScreen",
            Qt::QueuedConnection, Q_ARG(const std::string&, data));
    }
}

void ClientCallbacksHandler::onMaximumCallingTimeReached()
{
    if (m_mainWindow) {
        QMetaObject::invokeMethod(m_mainWindow, "onMaximumCallingTimeReached",
            Qt::QueuedConnection);
    }
}

void ClientCallbacksHandler::onCallingAccepted()
{
    if (m_mainWindow) {
        QMetaObject::invokeMethod(m_mainWindow, "onCallingAccepted",
            Qt::QueuedConnection);
    }
}

void ClientCallbacksHandler::onCallingDeclined()
{
    if (m_mainWindow) {
        QMetaObject::invokeMethod(m_mainWindow, "onCallingDeclined",
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

void ClientCallbacksHandler::onIncomingCallExpired(const std::string& friendNickname)
{
    if (m_mainWindow) {
        QString qNickname = QString::fromStdString(friendNickname);
        QMetaObject::invokeMethod(m_mainWindow, "onIncomingCallExpired",
            Qt::QueuedConnection, Q_ARG(const QString&, qNickname));
    }
}

void ClientCallbacksHandler::onNetworkError()
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

void ClientCallbacksHandler::onRemoteUserEndedCall()
{
    if (m_mainWindow) {
        QMetaObject::invokeMethod(m_mainWindow, "onRemoteUserEndedCall",
            Qt::QueuedConnection);
    }
}