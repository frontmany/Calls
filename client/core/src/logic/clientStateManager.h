#pragma once

#include <chrono>
#include <map>
#include <mutex>
#include <string>
#include <unordered_map>
#include <functional>
#include <optional>

#include "models/call.h"
#include "models/incomingCall.h"
#include "models/outgoingCall.h"
#include "models/meeting.h"
#include "models/outgoingJoinMeetingRequest.h"
#include "models/incomingJoinMeetingRequest.h"
#include "media/mediaState.h"
#include "media/mediaType.h"

namespace core::logic
{
    class ClientStateManager {
    public:
        ClientStateManager();
        ~ClientStateManager() = default;

        // ——— Authorization & connection ———
        bool isAuthorized() const;
        void setAuthorized(bool value);
        bool isConnectionDown() const;
        void setConnectionDown(bool value);

        // ——— User identity ———
        const std::string& getMyNickname() const;
        void setMyNickname(const std::string& nickname);
        void resetMyNickname();
        const std::string& getMyToken() const;
        void setMyToken(const std::string& token);
        void resetMyToken();

        // ——— Media state ———
        const media::MediaState getMediaState(media::MediaType type) const;
        void setMediaState(media::MediaType type, media::MediaState state);
        bool isViewingRemoteScreen() const;
        void setViewingRemoteScreen(bool value);
        bool isViewingRemoteCamera() const;
        void setViewingRemoteCamera(bool value);

        // ——— Outgoing call ———
        bool isOutgoingCall() const;
        std::optional<std::reference_wrapper<const core::OutgoingCall>> getOutgoingCall() const;
        template <typename Rep, typename Period>
        void setOutgoingCall(const std::string& nickname,
            const std::chrono::duration<Rep, Period>& timeout,
            std::function<void()> onTimeout)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_outgoingCall.has_value()) {
                m_outgoingCall->stop();
            }
            m_outgoingCall.emplace(nickname, timeout, std::move(onTimeout));
        }
        void resetOutgoingCall();

        // ——— Active call ———
        bool isActiveCall() const;
        bool isCallParticipantConnectionDown() const;
        void setCallParticipantConnectionDown(bool value);
        std::optional<std::reference_wrapper<const core::Call>> getActiveCall() const;
        void setActiveCall(const std::string& nickname,
            const CryptoPP::RSA::PublicKey& publicKey, const CryptoPP::SecByteBlock& callKey);
        void resetActiveCall();

        // ——— Incoming calls ———
        bool isIncomingCalls() const;
        const std::unordered_map<std::string, core::IncomingCall>& getIncomingCalls() const;
        int getIncomingCallsCount() const;
        template <typename Rep, typename Period>
        void addIncomingCall(const std::string& nickname,
            const CryptoPP::RSA::PublicKey& publicKey,
            const CryptoPP::SecByteBlock& callKey,
            const std::chrono::duration<Rep, Period>& timeout,
            std::function<void()> onTimeout)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            core::IncomingCall incomingCall(nickname, publicKey, callKey, timeout, std::move(onTimeout));
            m_incomingCalls.emplace(nickname, std::move(incomingCall));
        }
        void removeIncomingCall(const std::string& nickname);
        void resetIncomingCalls();

        // ——— Active meeting ———
        bool isActiveMeeting() const;
        bool isInMeeting() const { return isActiveMeeting(); }
        bool isMeetingOwner() const;
        std::optional<std::reference_wrapper<const core::Meeting>> getActiveMeeting() const;
        void setActiveMeeting(const std::string& meetingId, const CryptoPP::SecByteBlock& meetingKey);
        void addMeetingParticipant(const core::User& user, bool isOwner);
        void removeMeetingParticipant(const std::string& nickname);
        void resetActiveMeeting();

        // ——— Outgoing join meeting request ———
        bool isOutgoingJoinMeetingRequest() const;
        bool isOutgoingJoinMeeting() const { return isOutgoingJoinMeetingRequest(); }
        std::optional<std::reference_wrapper<const core::OutgoingJoinMeetingRequest>> getOutgoingJoinMeetingRequest() const;
        template <typename Rep, typename Period>
        void setOutgoingJoinMeetingRequest(const std::string& meetingId,
            const std::chrono::duration<Rep, Period>& timeout,
            std::function<void()> onTimeout)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_outgoingJoinMeetingRequest.has_value()) {
                m_outgoingJoinMeetingRequest->stop();
            }
            m_outgoingJoinMeetingRequest.emplace(meetingId, timeout, std::move(onTimeout));
        }
        void resetOutgoingJoinMeetingRequest();

        // ——— Incoming meeting join requests ———
        const std::unordered_map<std::string, core::IncomingJoinMeetingRequest>& getIncomingMeetingJoinRequests() const;
        void addIncomingMeetingJoinRequest(const std::string& nickname, const CryptoPP::RSA::PublicKey& publicKey);
        void removeIncomingMeetingJoinRequest(const std::string& nickname);
        void resetIncomingMeetingJoinRequests();

        void reset();

    private:
        mutable std::mutex m_mutex;
        std::map<media::MediaType, media::MediaState> m_mediaState;
        bool m_authorized = false;
        bool m_connectionDown = false;
        bool m_viewingRemoteScreen = false;
        bool m_viewingRemoteCamera = false;
        std::string m_myNickname; 
        std::string m_myToken;

        std::optional<core::Call> m_activeCall;
        std::optional<core::OutgoingCall> m_outgoingCall;
        std::unordered_map<std::string, core::IncomingCall> m_incomingCalls;

        std::optional<core::Meeting> m_activeMeeting;
        std::optional<core::OutgoingJoinMeetingRequest> m_outgoingJoinMeetingRequest;
        std::unordered_map<std::string, core::IncomingJoinMeetingRequest> m_incomingMeetingJoinRequests;
    };
}