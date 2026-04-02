#include "coreEventListener.h"
#include <QApplication>
#include <QByteArray>
#include <QStringList>
#include <utility>
#include <vector>

#include "logic/authorizationManager.h"
#include "logic/callManager.h"
#include "logic/meetingManager.h"
#include "logic/coreNetworkErrorHandler.h"
#include "logic/videoFrameCoalescer.h"
#include "utilities/logger.h"

CoreEventListener::CoreEventListener(
    AuthorizationManager* authorizationManager,
    CallManager* callManager,
    MeetingManager* meetingManager,
    CoreNetworkErrorHandler* networkErrorHandler,
    std::function<void(std::function<void()>)> scheduleUiFlush)
    : m_authorizationManager(authorizationManager)
    , m_callManager(callManager)
    , m_meetingManager(meetingManager)
    , m_coreNetworkErrorHandler(networkErrorHandler)
{
    m_videoFrameCoalescer = std::make_unique<core::VideoFrameCoalescer>(
        std::move(scheduleUiFlush),
        [this](const core::VideoFrameBuffer& f) { deliverIncomingScreen(f); },
        [this](const core::VideoFrameBuffer& f) { deliverLocalScreen(f); },
        [this](const core::VideoFrameBuffer& f) { deliverLocalCamera(f); },
        [this](const core::VideoFrameBuffer& f, const std::string& n) { deliverIncomingCamera(f, n); });
}

CoreEventListener::~CoreEventListener() = default;

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

void CoreEventListener::onIncomingScreenSharingStarted(const std::string& sharerNickname) {
    const QString sharerQ = QString::fromStdString(sharerNickname);
    if (m_meetingManager && m_meetingManager->isInMeeting()) {
        QMetaObject::invokeMethod(m_meetingManager, "onIncomingScreenSharingStarted",
            Qt::QueuedConnection, Q_ARG(QString, sharerQ));
        return;
    }
    if (m_callManager) {
        QMetaObject::invokeMethod(m_callManager, "onIncomingScreenSharingStarted",
            Qt::QueuedConnection);
    }
}

void CoreEventListener::onIncomingScreenSharingStopped(const std::string& sharerNickname) {
    const QString sharerQ = QString::fromStdString(sharerNickname);
    if (m_meetingManager && m_meetingManager->isInMeeting()) {
        QMetaObject::invokeMethod(m_meetingManager, "onIncomingScreenSharingStopped",
            Qt::QueuedConnection, Q_ARG(QString, sharerQ));
        return;
    }
    if (m_callManager) {
        QMetaObject::invokeMethod(m_callManager, "onIncomingScreenSharingStopped",
            Qt::QueuedConnection);
    }
}

void CoreEventListener::onIncomingScreen(const core::VideoFrameBuffer& frame) {
    if (m_videoFrameCoalescer)
        m_videoFrameCoalescer->pushIncomingScreen(frame);
}

void CoreEventListener::deliverIncomingScreen(const core::VideoFrameBuffer& frame) {
    QByteArray bytes(reinterpret_cast<const char*>(frame.data.data()), static_cast<int>(frame.data.size()));
    const quint8 fmt = static_cast<quint8>(frame.format);
    if (m_meetingManager && m_meetingManager->isInMeeting()) {
        QMetaObject::invokeMethod(m_meetingManager, "onIncomingScreenFrame",
            Qt::AutoConnection,
            Q_ARG(QByteArray, bytes),
            Q_ARG(int, frame.width),
            Q_ARG(int, frame.height),
            Q_ARG(quint8, fmt),
            Q_ARG(int, frame.strideY),
            Q_ARG(int, frame.strideUV),
            Q_ARG(int, frame.uvOffset));
        return;
    }
    if (m_callManager) {
        QMetaObject::invokeMethod(m_callManager, "onIncomingScreenFrame",
            Qt::AutoConnection,
            Q_ARG(QByteArray, bytes),
            Q_ARG(int, frame.width),
            Q_ARG(int, frame.height),
            Q_ARG(quint8, fmt),
            Q_ARG(int, frame.strideY),
            Q_ARG(int, frame.strideUV),
            Q_ARG(int, frame.uvOffset));
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

void CoreEventListener::onLocalScreen(const core::VideoFrameBuffer& frame) {
    if (m_videoFrameCoalescer)
        m_videoFrameCoalescer->pushLocalScreen(frame);
}

void CoreEventListener::deliverLocalScreen(const core::VideoFrameBuffer& frame) {
    QByteArray bytes(reinterpret_cast<const char*>(frame.data.data()), static_cast<int>(frame.data.size()));
    const quint8 fmt = static_cast<quint8>(frame.format);
    if (m_meetingManager && m_meetingManager->isInMeeting()) {
        QMetaObject::invokeMethod(m_meetingManager, "onLocalScreenFrame",
            Qt::AutoConnection,
            Q_ARG(QByteArray, bytes),
            Q_ARG(int, frame.width),
            Q_ARG(int, frame.height),
            Q_ARG(quint8, fmt),
            Q_ARG(int, frame.strideY),
            Q_ARG(int, frame.strideUV),
            Q_ARG(int, frame.uvOffset));
        return;
    }
    if (m_callManager) {
        QMetaObject::invokeMethod(m_callManager, "onLocalScreenFrame",
            Qt::AutoConnection,
            Q_ARG(QByteArray, bytes),
            Q_ARG(int, frame.width),
            Q_ARG(int, frame.height),
            Q_ARG(quint8, fmt),
            Q_ARG(int, frame.strideY),
            Q_ARG(int, frame.strideUV),
            Q_ARG(int, frame.uvOffset));
    }
}

void CoreEventListener::onIncomingCameraSharingStarted(const std::string& nickname) {
    const QString qNickname = QString::fromStdString(nickname);
    if (m_meetingManager && m_meetingManager->isInMeeting()) {
        QMetaObject::invokeMethod(m_meetingManager, "onIncomingCameraSharingStarted",
            Qt::QueuedConnection, Q_ARG(QString, qNickname));
        return;
    }
    if (m_callManager) {
        QMetaObject::invokeMethod(m_callManager, "onIncomingCameraSharingStarted",
            Qt::QueuedConnection, Q_ARG(QString, qNickname));
    }
}

void CoreEventListener::onIncomingCameraSharingStopped(const std::string& nickname) {
    if (m_videoFrameCoalescer)
        m_videoFrameCoalescer->dropPendingRemoteCamera(nickname);
    const QString qNickname = QString::fromStdString(nickname);
    if (m_meetingManager && m_meetingManager->isInMeeting()) {
        QMetaObject::invokeMethod(m_meetingManager, "onIncomingCameraSharingStopped",
            Qt::QueuedConnection, Q_ARG(QString, qNickname));
        return;
    }
    if (m_callManager) {
        QMetaObject::invokeMethod(m_callManager, "onIncomingCameraSharingStopped",
            Qt::QueuedConnection, Q_ARG(QString, qNickname));
    }
}

void CoreEventListener::onIncomingCamera(const core::VideoFrameBuffer& frame, const std::string& nickname) {
    if (m_videoFrameCoalescer)
        m_videoFrameCoalescer->pushIncomingCamera(frame, nickname);
}

void CoreEventListener::deliverIncomingCamera(const core::VideoFrameBuffer& frame, const std::string& nickname) {
    QByteArray bytes(reinterpret_cast<const char*>(frame.data.data()), static_cast<int>(frame.data.size()));
    const quint8 fmt = static_cast<quint8>(frame.format);
    if (m_meetingManager && m_meetingManager->isInMeeting()) {
        QMetaObject::invokeMethod(m_meetingManager, "onIncomingCameraFrame",
            Qt::AutoConnection,
            Q_ARG(QByteArray, bytes),
            Q_ARG(int, frame.width),
            Q_ARG(int, frame.height),
            Q_ARG(quint8, fmt),
            Q_ARG(int, frame.strideY),
            Q_ARG(int, frame.strideUV),
            Q_ARG(int, frame.uvOffset),
            Q_ARG(QString, QString::fromStdString(nickname)));
        return;
    }
    if (m_callManager) {
        QMetaObject::invokeMethod(m_callManager, "onIncomingCameraFrame",
            Qt::AutoConnection,
            Q_ARG(QByteArray, bytes),
            Q_ARG(int, frame.width),
            Q_ARG(int, frame.height),
            Q_ARG(quint8, fmt),
            Q_ARG(int, frame.strideY),
            Q_ARG(int, frame.strideUV),
            Q_ARG(int, frame.uvOffset));
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

void CoreEventListener::onLocalCamera(const core::VideoFrameBuffer& frame) {
    if (m_videoFrameCoalescer)
        m_videoFrameCoalescer->pushLocalCamera(frame);
}

void CoreEventListener::deliverLocalCamera(const core::VideoFrameBuffer& frame) {
    QByteArray bytes(reinterpret_cast<const char*>(frame.data.data()), static_cast<int>(frame.data.size()));
    const quint8 fmt = static_cast<quint8>(frame.format);
    if (m_meetingManager && m_meetingManager->isInMeeting()) {
        QMetaObject::invokeMethod(m_meetingManager, "onLocalCameraFrame",
            Qt::AutoConnection,
            Q_ARG(QByteArray, bytes),
            Q_ARG(int, frame.width),
            Q_ARG(int, frame.height),
            Q_ARG(quint8, fmt),
            Q_ARG(int, frame.strideY),
            Q_ARG(int, frame.strideUV),
            Q_ARG(int, frame.uvOffset));
        return;
    }
    if (m_callManager) {
        QMetaObject::invokeMethod(m_callManager, "onLocalCameraFrame",
            Qt::AutoConnection,
            Q_ARG(QByteArray, bytes),
            Q_ARG(int, frame.width),
            Q_ARG(int, frame.height),
            Q_ARG(quint8, fmt),
            Q_ARG(int, frame.strideY),
            Q_ARG(int, frame.strideUV),
            Q_ARG(int, frame.uvOffset));
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
    const bool inMeeting = m_meetingManager && m_meetingManager->isInMeeting();
    LOG_INFO("CoreEventListener incoming call: {} (inMeeting={})", friendNickname, inMeeting);
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

void CoreEventListener::onCallParticipantSpeaking(const std::string& nickname, bool speaking)
{
    if (m_callManager) {
        QMetaObject::invokeMethod(m_callManager, "onCallParticipantSpeaking",
            Qt::QueuedConnection,
            Q_ARG(QString, QString::fromStdString(nickname)),
            Q_ARG(bool, speaking));
    }
}

void CoreEventListener::onCallParticipantMuted(const std::string& nickname, bool muted)
{
    if (m_callManager) {
        QMetaObject::invokeMethod(m_callManager, "onCallParticipantMuted",
            Qt::QueuedConnection,
            Q_ARG(QString, QString::fromStdString(nickname)),
            Q_ARG(bool, muted));
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
    if (m_videoFrameCoalescer)
        m_videoFrameCoalescer->dropPendingRemoteCamera(nickname);
    if (m_meetingManager) {
        QMetaObject::invokeMethod(m_meetingManager, "onMeetingParticipantLeft",
            Qt::QueuedConnection, Q_ARG(QString, QString::fromStdString(nickname)));
    }
}

void CoreEventListener::onMeetingParticipantConnectionDown(const std::string& nickname)
{
    if (m_meetingManager) {
        QMetaObject::invokeMethod(m_meetingManager, "onMeetingParticipantConnectionDown",
            Qt::QueuedConnection, Q_ARG(QString, QString::fromStdString(nickname)));
    }
}

void CoreEventListener::onMeetingParticipantConnectionRestored(const std::string& nickname)
{
    if (m_meetingManager) {
        QMetaObject::invokeMethod(m_meetingManager, "onMeetingParticipantConnectionRestored",
            Qt::QueuedConnection, Q_ARG(QString, QString::fromStdString(nickname)));
    }
}

void CoreEventListener::onMeetingParticipantSpeaking(const std::string& nickname, bool speaking)
{
    if (m_meetingManager) {
        QMetaObject::invokeMethod(m_meetingManager, "onMeetingParticipantSpeaking",
            Qt::QueuedConnection,
            Q_ARG(QString, QString::fromStdString(nickname)),
            Q_ARG(bool, speaking));
    }
}

void CoreEventListener::onMeetingParticipantMuted(const std::string& nickname, bool muted)
{
    if (m_meetingManager) {
        QMetaObject::invokeMethod(m_meetingManager, "onMeetingParticipantMuted",
            Qt::QueuedConnection,
            Q_ARG(QString, QString::fromStdString(nickname)),
            Q_ARG(bool, muted));
    }
}

void CoreEventListener::onMeetingRosterResynced(const std::vector<std::string>& participants)
{
    if (!m_meetingManager) {
        return;
    }

    QStringList list;
    list.reserve(static_cast<int>(participants.size()));
    for (const auto& p : participants) {
        list << QString::fromStdString(p);
    }

    QMetaObject::invokeMethod(m_meetingManager, "onMeetingRosterResynced",
        Qt::QueuedConnection,
        Q_ARG(QStringList, list));
}
