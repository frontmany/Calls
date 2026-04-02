#pragma once 

#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <system_error>

#include "eventListener.h"

class AuthorizationManager;
class CallManager;
class MeetingManager;
class CoreNetworkErrorHandler;

namespace core {
class VideoFrameCoalescer;
}

class CoreEventListener : public core::EventListener {
public:
    CoreEventListener(
        AuthorizationManager* authorizationManager,
        CallManager* callManager,
        MeetingManager* meetingManager,
        CoreNetworkErrorHandler* networkErrorHandler,
        std::function<void(std::function<void()>)> scheduleUiFlush);

    ~CoreEventListener() override;

    void onAuthorizationResult(std::error_code ec) override;
    void onStartOutgoingCallResult(std::error_code ec) override;

    void onIncomingScreenSharingStarted(const std::string& sharerNickname) override;
    void onIncomingScreenSharingStopped(const std::string& sharerNickname) override;
    void onIncomingScreen(const core::VideoFrameBuffer& frame) override;

    void onStartScreenSharingError() override;
    void onLocalScreen(const core::VideoFrameBuffer& frame) override;

    void onIncomingCameraSharingStarted(const std::string& nickname) override;
    void onIncomingCameraSharingStopped(const std::string& nickname) override;
    void onIncomingCamera(const core::VideoFrameBuffer& frame, const std::string& nickname) override;

    void onStartCameraSharingError() override;
    void onLocalCamera(const core::VideoFrameBuffer& frame) override;

    void onOutgoingCallAccepted(const std::string& nickname) override;
    void onOutgoingCallDeclined() override;
    void onOutgoingCallTimeout(std::error_code ec) override;
    void onIncomingCall(const std::string& friendNickname) override;
    void onIncomingCallExpired(std::error_code ec, const std::string& friendNickname) override;
    void onCallEndedByRemote(std::error_code ec) override;

    void onCallParticipantConnectionDown() override;
    void onCallParticipantConnectionRestored() override;
    void onCallParticipantSpeaking(const std::string& nickname, bool speaking) override;
    void onCallParticipantMuted(const std::string& nickname, bool muted) override;

    void onMeetingCreated(const std::string& meetingId) override;
    void onMeetingCreateRejected(std::error_code ec) override;
    void onMeetingJoinRequestReceived(const std::string& friendNickname) override;
    void onMeetingJoinRequestCancelled(const std::string& friendNickname) override;
    void onJoinMeetingAccepted(const std::string& meetingId, const std::vector<std::string>& participants) override;
    void onJoinMeetingDeclined(const std::string& meetingId) override;
    void onMeetingNotFound() override;
    void onJoinMeetingRequestTimeout() override;
    void onMeetingEndedByOwner() override;
    void onMeetingParticipantJoined(const std::string& nickname) override;
    void onMeetingParticipantLeft(const std::string& nickname) override;
    void onMeetingParticipantConnectionDown(const std::string& nickname) override;
    void onMeetingParticipantConnectionRestored(const std::string& nickname) override;
    void onMeetingParticipantSpeaking(const std::string& nickname, bool speaking) override;
    void onMeetingParticipantMuted(const std::string& nickname, bool muted) override;
    void onMeetingRosterResynced(const std::vector<std::string>& participants) override;

    void onConnectionDown() override;
    void onConnectionEstablished() override;
    void onConnectionEstablishedAuthorizationNeeded() override;

private:
    void deliverIncomingScreen(const core::VideoFrameBuffer& frame);
    void deliverLocalScreen(const core::VideoFrameBuffer& frame);
    void deliverIncomingCamera(const core::VideoFrameBuffer& frame, const std::string& nickname);
    void deliverLocalCamera(const core::VideoFrameBuffer& frame);

    AuthorizationManager* m_authorizationManager;
    CallManager* m_callManager;
    MeetingManager* m_meetingManager;
    CoreNetworkErrorHandler* m_coreNetworkErrorHandler;
    std::unique_ptr<core::VideoFrameCoalescer> m_videoFrameCoalescer;
};