#include "CallsClientHandler.h"
#include <QMetaObject>
#include <QApplication>
#include "mainWindow.h"

CallsClientHandler::CallsClientHandler(MainWindow* mainWindow)
    : m_mainWindow(mainWindow)
{
}

void CallsClientHandler::onAuthorizationResult(calls::ErrorCode ec)
{
    if (m_mainWindow) {
        QMetaObject::invokeMethod(m_mainWindow, "onAuthorizationResult",
            Qt::QueuedConnection, Q_ARG(calls::ErrorCode, ec));
    }
}

void CallsClientHandler::onStartCallingResult(calls::ErrorCode ec)
{
    if (m_mainWindow) {
        QMetaObject::invokeMethod(m_mainWindow, "onStartCallingResult",
            Qt::QueuedConnection, Q_ARG(calls::ErrorCode, ec));
    }
}

void CallsClientHandler::onAcceptCallResult(calls::ErrorCode ec, const std::string& nickname)
{
    if (m_mainWindow) {
        QString qNickname = QString::fromStdString(nickname);
        QMetaObject::invokeMethod(m_mainWindow, "onAcceptCallResult",
            Qt::QueuedConnection,
            Q_ARG(calls::ErrorCode, ec),
            Q_ARG(const QString&, qNickname));
    }
}

void CallsClientHandler::onMaximumCallingTimeReached()
{
    if (m_mainWindow) {
        QMetaObject::invokeMethod(m_mainWindow, "onMaximumCallingTimeReached",
            Qt::QueuedConnection);
    }
}

void CallsClientHandler::onCallingAccepted()
{
    if (m_mainWindow) {
        QMetaObject::invokeMethod(m_mainWindow, "onCallingAccepted",
            Qt::QueuedConnection);
    }
}

void CallsClientHandler::onCallingDeclined()
{
    if (m_mainWindow) {
        QMetaObject::invokeMethod(m_mainWindow, "onCallingDeclined",
            Qt::QueuedConnection);
    }
}

void CallsClientHandler::onIncomingCall(const std::string& friendNickname)
{
    if (m_mainWindow) {
        QString qFriendNickname = QString::fromStdString(friendNickname);
        QMetaObject::invokeMethod(m_mainWindow, "onIncomingCall",
            Qt::QueuedConnection, Q_ARG(const QString&, qFriendNickname));
    }
}

void CallsClientHandler::onIncomingCallExpired(const std::string& friendNickname)
{
    if (m_mainWindow) {
        QString qNickname = QString::fromStdString(friendNickname);
        QMetaObject::invokeMethod(m_mainWindow, "onIncomingCallExpired",
            Qt::QueuedConnection, Q_ARG(const QString&, qNickname));
    }
}

void CallsClientHandler::onNetworkError()
{
    if (m_mainWindow) {
        QMetaObject::invokeMethod(m_mainWindow, "onNetworkError",
            Qt::QueuedConnection);
    }
}

void CallsClientHandler::onConnectionRestored()
{
    if (m_mainWindow) {
        QMetaObject::invokeMethod(m_mainWindow, "onConnectionRestored",
            Qt::QueuedConnection);
    }
}

void CallsClientHandler::onRemoteUserEndedCall()
{
    if (m_mainWindow) {
        QMetaObject::invokeMethod(m_mainWindow, "onRemoteUserEndedCall",
            Qt::QueuedConnection);
    }
}