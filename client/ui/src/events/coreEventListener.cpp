#include "coreEventListener.h"
#include <QApplication>
#include <QByteArray>
#include <QStringList>
#include <vector>

#include "logic/authorizationManager.h"
#include "logic/callManager.h"
#include "logic/meetingManager.h"
#include "logic/coreNetworkErrorHandler.h"

CoreEventListener::CoreEventListener(AuthorizationManager* authorizationManager, CallManager* callManager, MeetingManager* meetingManager, CoreNetworkErrorHandler* networkErrorHandler)
    : m_authorizationManager(authorizationManager)
    , m_callManager(callManager)
    , m_meetingManager(meetingManager)
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
        QMetaObject::invokeMethod(m_callManager, "onStartOutgoingCallResult",
            Qt::QueuedConnection, Q_ARG(std::error_code, ec));
    }
}

void CoreEventListener::onIncomingScreenSharingStarted() {
    if (m_meetingManager && m_meetingManager->isInMeeting()) {
        QMetaObject::invokeMethod(m_meetingManager, "onIncomingScreenSharingStarted",
            Qt::QueuedConnection);
        return;
    }
    if (m_callManager) {
        QMetaObject::invokeMethod(m_callManager, "onIncomingScreenSharingStarted",
            Qt::QueuedConnection);
    }
}

void CoreEventListener::onIncomingScreenSharingStopped() {
    if (m_meetingManager && m_meetingManager->isInMeeting()) {
        QMetaObject::invokeMethod(m_meetingManager, "onIncomingScreenSharingStopped",
            Qt::QueuedConnection);
        return;
    }
    if (m_callManager) {
        QMetaObject::invokeMethod(m_callManager, "onIncomingScreenSharingStopped",
            Qt::QueuedConnection);
    }
}

void CoreEventListener::onIncomingScreen(const std::vector<unsigned char>& data, int width, int height) {
    QByteArray frameData(reinterpret_cast<const char*>(data.data()), static_cast<int>(data.size()));
    if (m_meetingManager && m_meetingManager->isInMeeting()) {
        QMetaObject::invokeMethod(m_meetingManager, "onIncomingScreenFrame",
            Qt::QueuedConnection, Q_ARG(QByteArray, frameData), Q_ARG(int, width), Q_ARG(int, height));
        return;
    }
    if (m_callManager) {
        QMetaObject::invokeMethod(m_callManager, "onIncomingScreenFrame",
            Qt::QueuedConnection, Q_ARG(QByteArray, frameData), Q_ARG(int, width), Q_ARG(int, height));
    }
}

void CoreEventListener::onStartScreenSharingError() {
    if (m_meetingManager && m_meetingManager->isInMeeting()) {
        QMetaObject::invokeMethod(m_meetingManager, "onStartScreenSharingError",
            Qt::QueuedConnection);
        return;
    }
    if (m_callManager) {
        QMetaObject::invokeMethod(m_callManager, "onStartScreenSharingError",
            Qt::QueuedConnection);
    }
}

void CoreEventListener::onLocalScreen(const std::vector<unsigned char>& data, int width, int height) {
    QByteArray frameData(reinterpret_cast<const char*>(data.data()), static_cast<int>(data.size()));
    if (m_meetingManager && m_meetingManager->isInMeeting()) {
        QMetaObject::invokeMethod(m_meetingManager, "onLocalScreenFrame",
            Qt::QueuedConnection, Q_ARG(QByteArray, frameData), Q_ARG(int, width), Q_ARG(int, height));
        return;
    }
    if (m_callManager) {
        QMetaObject::invokeMethod(m_callManager, "onLocalScreenFrame",
            Qt::QueuedConnection, Q_ARG(QByteArray, frameData), Q_ARG(int, width), Q_ARG(int, height));
    }
}

void CoreEventListener::onIncomingCameraSharingStarted() {
    if (m_meetingManager && m_meetingManager->isInMeeting()) {
        QMetaObject::invokeMethod(m_meetingManager, "onIncomingCameraSharingStarted",
            Qt::QueuedConnection);
        return;
    }
    if (m_callManager) {
        QMetaObject::invokeMethod(m_callManager, "onIncomingCameraSharingStarted",
            Qt::QueuedConnection);
    }
}

void CoreEventListener::onIncomingCameraSharingStopped() {
    if (m_meetingManager && m_meetingManager->isInMeeting()) {
        QMetaObject::invokeMethod(m_meetingManager, "onIncomingCameraSharingStopped",
            Qt::QueuedConnection);
        return;
    }
    if (m_callManager) {
        QMetaObject::invokeMethod(m_callManager, "onIncomingCameraSharingStopped",
            Qt::QueuedConnection);
    }
}

void CoreEventListener::onIncomingCamera(const std::vector<unsigned char>& data, int width, int height, const std::string& nickname) {
    QByteArray frameData(reinterpret_cast<const char*>(data.data()), static_cast<int>(data.size()));
    if (m_meetingManager && m_meetingManager->isInMeeting()) {
        QMetaObject::invokeMethod(m_meetingManager, "onIncomingCameraFrame",
            Qt::QueuedConnection,
            Q_ARG(QByteArray, frameData),
            Q_ARG(int, width),
            Q_ARG(int, height),
            Q_ARG(QString, QString::fromStdString(nickname)));
        return;
    }
    if (m_callManager) {
        QMetaObject::invokeMethod(m_callManager, "onIncomingCameraFrame",
            Qt::QueuedConnection, Q_ARG(QByteArray, frameData), Q_ARG(int, width), Q_ARG(int, height));
    }
}

void CoreEventListener::onStartCameraSharingError() {
    if (m_meetingManager && m_meetingManager->isInMeeting()) {
        QMetaObject::invokeMethod(m_meetingManager, "onStartCameraSharingError",
            Qt::QueuedConnection);
        return;
    }
    if (m_callManager) {
        QMetaObject::invokeMethod(m_callManager, "onStartCameraSharingError",
            Qt::QueuedConnection);
    }
}

void CoreEventListener::onLocalCamera(const std::vector<unsigned char>& data, int width, int height) {
    QByteArray frameData(reinterpret_cast<const char*>(data.data()), static_cast<int>(data.size()));
    if (m_meetingManager && m_meetingManager->isInMeeting()) {
        QMetaObject::invokeMethod(m_meetingManager, "onLocalCameraFrame",
            Qt::QueuedConnection, Q_ARG(QByteArray, frameData), Q_ARG(int, width), Q_ARG(int, height));
        return;
    }
    if (m_callManager) {
        QMetaObject::invokeMethod(m_callManager, "onLocalCameraFrame",
            Qt::QueuedConnection, Q_ARG(QByteArray, frameData), Q_ARG(int, width), Q_ARG(int, height));
    }
}

void CoreEventListener::onOutgoingCallAccepted(const std::string& nickname)
{
    if (m_callManager) {
        QString qNickname = QString::fromStdString(nickname);
        QMetaObject::invokeMethod(m_callManager, "onOutgoingCallAccepted",
            Qt::QueuedConnection, Q_ARG(QString, qNickname));
    }
}

void CoreEventListener::onOutgoingCallDeclined()
{
    if (m_callManager) {
        QMetaObject::invokeMethod(m_callManager, "onOutgoingCallDeclined",
            Qt::QueuedConnection);
    }
}

void CoreEventListener::onOutgoingCallTimeout(std::error_code ec)
{
    if (m_callManager) {
        QMetaObject::invokeMethod(m_callManager, "onMaximumOutgoingCallTimeReached",
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

void CoreEventListener::onConnectionEstablished()
{
    if (m_coreNetworkErrorHandler) {
        QMetaObject::invokeMethod(static_cast<QObject*>(m_coreNetworkErrorHandler), "onConnectionEstablished",
            Qt::QueuedConnection);
    }
}

void CoreEventListener::onConnectionEstablishedAuthorizationNeeded()
{
    if (m_coreNetworkErrorHandler) {
        QMetaObject::invokeMethod(static_cast<QObject*>(m_coreNetworkErrorHandler), "onConnectionEstablishedAuthorizationNeeded",
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

void CoreEventListener::onMeetingCreated(const std::string& meetingId)
{
    if (m_meetingManager) {
        QMetaObject::invokeMethod(m_meetingManager, "onMeetingCreated",
            Qt::QueuedConnection, Q_ARG(QString, QString::fromStdString(meetingId)));
    }
}

void CoreEventListener::onMeetingCreateRejected(std::error_code ec)
{
    if (m_meetingManager) {
        QMetaObject::invokeMethod(m_meetingManager, "onMeetingCreateRejected",
            Qt::QueuedConnection, Q_ARG(std::error_code, ec));
    }
}
void CoreEventListener::onMeetingJoinRequestReceived(const std::string& friendNickname)
{
    if (m_meetingManager) {
        QMetaObject::invokeMethod(m_meetingManager, "onMeetingJoinRequestReceived",
            Qt::QueuedConnection, Q_ARG(QString, QString::fromStdString(friendNickname)));
    }
}

void CoreEventListener::onMeetingJoinRequestCancelled(const std::string& friendNickname)
{
    if (m_meetingManager) {
        QMetaObject::invokeMethod(m_meetingManager, "onMeetingJoinRequestCancelled",
            Qt::QueuedConnection, Q_ARG(QString, QString::fromStdString(friendNickname)));
    }
}

void CoreEventListener::onJoinMeetingAccepted(const std::string& meetingId, const std::vector<std::string>& participants)
{
    if (m_meetingManager) {
        QStringList list;
        for (const auto& s : participants)
            list << QString::fromStdString(s);
        QMetaObject::invokeMethod(m_meetingManager, "onJoinMeetingAccepted",
            Qt::QueuedConnection,
            Q_ARG(QString, QString::fromStdString(meetingId)),
            Q_ARG(QStringList, list));
    }
}

void CoreEventListener::onJoinMeetingDeclined(const std::string& meetingId)
{
    if (m_meetingManager) {
        QMetaObject::invokeMethod(m_meetingManager, "onJoinMeetingDeclined",
            Qt::QueuedConnection, Q_ARG(QString, QString::fromStdString(meetingId)));
    }
}

void CoreEventListener::onMeetingNotFound()
{
    if (m_meetingManager) {
        QMetaObject::invokeMethod(m_meetingManager, "onMeetingNotFound", Qt::QueuedConnection);
    }
}

void CoreEventListener::onJoinMeetingRequestTimeout()
{
    if (m_meetingManager) {
        QMetaObject::invokeMethod(m_meetingManager, "onJoinMeetingRequestTimeout",
            Qt::QueuedConnection);
    }
}

void CoreEventListener::onMeetingEndedByOwner()
{
    if (m_meetingManager) {
        QMetaObject::invokeMethod(m_meetingManager, "onMeetingEndedByOwner", Qt::QueuedConnection);
    }
}

void CoreEventListener::onMeetingParticipantJoined(const std::string& nickname)
{
    if (m_meetingManager) {
        QMetaObject::invokeMethod(m_meetingManager, "onMeetingParticipantJoined",
            Qt::QueuedConnection, Q_ARG(QString, QString::fromStdString(nickname)));
    }
}

void CoreEventListener::onMeetingParticipantLeft(const std::string& nickname)
{
    if (m_meetingManager) {
        QMetaObject::invokeMethod(m_meetingManager, "onMeetingParticipantLeft",
            Qt::QueuedConnection, Q_ARG(QString, QString::fromStdString(nickname)));
    }
}
