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
    void onIncomingScreen(const std::vector<unsigned char>& data, int width, int height) override;

    void onStartScreenSharingError() override;
    void onLocalScreen(const std::vector<unsigned char>& data, int width, int height) override;

    void onIncomingCameraSharingStarted(const std::string& nickname) override;
    void onIncomingCameraSharingStopped(const std::string& nickname) override;
    void onIncomingCamera(const std::vector<unsigned char>& data, int width, int height, const std::string& nickname) override;

    void onStartCameraSharingError() override;
    void onLocalCamera(const std::vector<unsigned char>& data, int width, int height) override;

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
    void deliverIncomingScreen(const std::vector<unsigned char>& data, int width, int height);
    void deliverLocalScreen(const std::vector<unsigned char>& data, int width, int height);
    void deliverIncomingCamera(const std::vector<unsigned char>& data, int width, int height, const std::string& nickname);
    void deliverLocalCamera(const std::vector<unsigned char>& data, int width, int height);

    AuthorizationManager* m_authorizationManager;
    CallManager* m_callManager;
    MeetingManager* m_meetingManager;
    CoreNetworkErrorHandler* m_coreNetworkErrorHandler;
    std::unique_ptr<core::VideoFrameCoalescer> m_videoFrameCoalescer;
};