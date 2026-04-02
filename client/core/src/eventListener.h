#pragma once

#include <string>
#include <vector>
#include <system_error>

#include "videoFrameBuffer.h"

namespace core
{
    class EventListener {
    public:
        virtual ~EventListener() = default;

        virtual void onAuthorizationResult(std::error_code ec) = 0;
        virtual void onStartOutgoingCallResult(std::error_code ec) = 0;

        virtual void onIncomingScreenSharingStarted(const std::string& sharerNickname) = 0;
        virtual void onIncomingScreenSharingStopped(const std::string& sharerNickname) = 0;
        virtual void onIncomingScreen(const VideoFrameBuffer& frame) = 0;

        virtual void onStartScreenSharingError() = 0;
        virtual void onLocalScreen(const VideoFrameBuffer& frame) = 0;

        virtual void onIncomingCameraSharingStarted(const std::string& nickname) = 0;
        virtual void onIncomingCameraSharingStopped(const std::string& nickname) = 0;
        virtual void onIncomingCamera(const VideoFrameBuffer& frame, const std::string& nickname) = 0;

        virtual void onStartCameraSharingError() = 0;
        virtual void onLocalCamera(const VideoFrameBuffer& frame) = 0;

        virtual void onOutgoingCallAccepted(const std::string& nickname) = 0;
        virtual void onOutgoingCallDeclined() = 0;
        virtual void onOutgoingCallTimeout(std::error_code ec) = 0;

        virtual void onIncomingCall(const std::string& nickname) = 0;
        virtual void onIncomingCallExpired(std::error_code ec, const std::string& nickname) = 0;
        virtual void onCallEndedByRemote(std::error_code ec) = 0;

        virtual void onCallParticipantConnectionDown() = 0;
        virtual void onCallParticipantConnectionRestored() = 0;
        virtual void onCallParticipantSpeaking(const std::string& nickname, bool speaking) = 0;
        virtual void onCallParticipantMuted(const std::string& nickname, bool muted) = 0;

        virtual void onMeetingCreated(const std::string& meetingId) = 0;
        virtual void onMeetingCreateRejected(std::error_code ec) = 0;
        virtual void onMeetingJoinRequestReceived(const std::string& friendNickname) = 0;
        virtual void onMeetingJoinRequestCancelled(const std::string& friendNickname) = 0;
        virtual void onJoinMeetingAccepted(const std::string& meetingId, const std::vector<std::string>& participants) = 0;
        virtual void onJoinMeetingDeclined(const std::string& meetingId) = 0;
        virtual void onMeetingNotFound() = 0;
        virtual void onJoinMeetingRequestTimeout() = 0;
        virtual void onMeetingEndedByOwner() = 0;
        virtual void onMeetingParticipantJoined(const std::string& nickname) = 0;
        virtual void onMeetingParticipantLeft(const std::string& nickname) = 0;
        virtual void onMeetingParticipantConnectionDown(const std::string& nickname) = 0;
        virtual void onMeetingParticipantConnectionRestored(const std::string& nickname) = 0;
        virtual void onMeetingParticipantSpeaking(const std::string& nickname, bool speaking) = 0;
        virtual void onMeetingParticipantMuted(const std::string& nickname, bool muted) = 0;
        virtual void onMeetingRosterResynced(const std::vector<std::string>& participants) = 0;

        virtual void onConnectionDown() = 0;
        virtual void onConnectionEstablished() = 0;
        virtual void onConnectionEstablishedAuthorizationNeeded() = 0;
    };
}