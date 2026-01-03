#include "calliforniaEventListener.h"
#include <QApplication>
#include <QMetaObject>
#include <QMetaType>

#include <vector>

#include "mainWindow.h"

Q_DECLARE_METATYPE(std::vector<unsigned char>)

ClientCallbacksHandler::ClientCallbacksHandler(MainWindow* mainWindow)
    : m_mainWindow(mainWindow)
{
    qRegisterMetaType<std::vector<unsigned char>>("std::vector<unsigned char>");
}

void ClientCallbacksHandler::onAuthorizationResult(callifornia::ErrorCode ec)
{
    if (m_mainWindow) {
        QMetaObject::invokeMethod(m_mainWindow, "onAuthorizationResult",
            Qt::QueuedConnection, Q_ARG(callifornia::ErrorCode, ec));
    }
}

void ClientCallbacksHandler::onStartCallingResult(callifornia::ErrorCode ec)
{
    if (m_mainWindow) {
        QMetaObject::invokeMethod(m_mainWindow, "onStartCallingResult",
            Qt::QueuedConnection, Q_ARG(callifornia::ErrorCode, ec));
    }
}

void ClientCallbacksHandler::onAcceptCallResult(callifornia::ErrorCode ec, const std::string& nickname)
{
    if (m_mainWindow) {
        QString qNickname = QString::fromStdString(nickname);
        QMetaObject::invokeMethod(m_mainWindow, "onAcceptCallResult",
            Qt::QueuedConnection,
            Q_ARG(callifornia::ErrorCode, ec),
            Q_ARG(const QString&, qNickname));
    }
}

void ClientCallbacksHandler::onScreenSharingStarted() {
    if (m_mainWindow) {
        QMetaObject::invokeMethod(m_mainWindow, "onScreenSharingStarted",
            Qt::QueuedConnection);
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

void ClientCallbacksHandler::onIncomingScreen(const std::vector<unsigned char>& data) {
    if (m_mainWindow) {
        QMetaObject::invokeMethod(m_mainWindow, "onIncomingScreen",
            Qt::QueuedConnection, Q_ARG(const std::vector<unsigned char>&, data));
    }
}

void ClientCallbacksHandler::onCameraSharingStarted() {
    if (m_mainWindow) {
        QMetaObject::invokeMethod(m_mainWindow, "onCameraSharingStarted",
            Qt::QueuedConnection);
    }
}

void ClientCallbacksHandler::onStartCameraSharingError() {
    if (m_mainWindow) {
        QMetaObject::invokeMethod(m_mainWindow, "onStartCameraSharingError",
            Qt::QueuedConnection);
    }
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