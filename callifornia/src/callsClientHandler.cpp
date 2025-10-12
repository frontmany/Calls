#include "CallsClientHandler.h"
#include <QMetaObject>
#include <QApplication>

#include "mainWindow.h"

CallsClientHandler::CallsClientHandler(MainWindow* mainWindow)
    : m_mainWindow(mainWindow)
{
}

void CallsClientHandler::onAuthorizationResult(bool success)
{
    if (m_mainWindow) {
        QMetaObject::invokeMethod(m_mainWindow, "onAuthorizationResult",
            Qt::QueuedConnection, Q_ARG(bool, success));
    }
}

void CallsClientHandler::onLogoutResult(bool success)
{
    if (m_mainWindow) {
        QMetaObject::invokeMethod(m_mainWindow, "onLogoutResult",
            Qt::QueuedConnection, Q_ARG(bool, success));
    }
}

void CallsClientHandler::onShutdownResult(bool success)
{
    if (m_mainWindow) {
        QMetaObject::invokeMethod(m_mainWindow, "onShutdownResult",
            Qt::QueuedConnection, Q_ARG(bool, success));
    }
}

void CallsClientHandler::onStartCallingResult(bool success)
{
    if (m_mainWindow) {
        QMetaObject::invokeMethod(m_mainWindow, "onStartCallingResult",
            Qt::QueuedConnection, Q_ARG(bool, success));
    }
}

void CallsClientHandler::onDeclineIncomingCallResult(bool success, const std::string& nickname)
{
    if (m_mainWindow) {
        QString qNickname = QString::fromStdString(nickname);
        QMetaObject::invokeMethod(m_mainWindow, "onDeclineIncomingCallResult",
            Qt::QueuedConnection,
            Q_ARG(bool, success),
            Q_ARG(QString, qNickname));
    }
}

void CallsClientHandler::onAllIncomingCallsDeclinedResult(bool success) {
    if (m_mainWindow) {
        QMetaObject::invokeMethod(m_mainWindow, "onAllIncomingCallsDeclinedResult",
            Qt::QueuedConnection,
            Q_ARG(bool, success));
    }
}

void CallsClientHandler::onAcceptIncomingCallResult(bool success, const std::string& nickname)
{
    if (m_mainWindow) {
        QString qNickname = QString::fromStdString(nickname);
        QMetaObject::invokeMethod(m_mainWindow, "onAcceptIncomingCallResult",
            Qt::QueuedConnection,
            Q_ARG(bool, success),
            Q_ARG(QString, qNickname));
    }
}

void CallsClientHandler::onEndCallResult(bool success)
{
    if (m_mainWindow) {
        QMetaObject::invokeMethod(m_mainWindow, "onEndCallResult",
            Qt::QueuedConnection, Q_ARG(bool, success));
    }
}

void CallsClientHandler::onCallingStoppedResult(bool success)
{
    if (m_mainWindow) {
        QMetaObject::invokeMethod(m_mainWindow, "onCallingStoppedResult",
            Qt::QueuedConnection, Q_ARG(bool, success));
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

void CallsClientHandler::onIncomingCall(const std::string& callId)
{
    if (m_mainWindow) {
        QString qCallId = QString::fromStdString(callId);
        QMetaObject::invokeMethod(m_mainWindow, "onIncomingCall",
            Qt::QueuedConnection, Q_ARG(QString, qCallId));
    }
}

void CallsClientHandler::onIncomingCallExpired(const std::string& friendNickname)
{
    if (m_mainWindow) {
        QString qNickname = QString::fromStdString(friendNickname);
        QMetaObject::invokeMethod(m_mainWindow, "onIncomingCallEnded",
            Qt::QueuedConnection, Q_ARG(QString, qNickname));
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