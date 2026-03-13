#pragma once

#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace server
{
    class User;
    class PendingMeetingJoinRequest;

    typedef std::shared_ptr<User> UserPtr;
    typedef std::shared_ptr<PendingMeetingJoinRequest> PendingMeetingJoinRequestPtr;

    class Meeting {
    public:
        struct ParticipantInfo {
            UserPtr user;
            std::string encryptedNickname;
        };

        Meeting(const std::string& meetingId, const std::string& meetingIdHash, const UserPtr& owner);

        const std::string& getMeetingId() const;
        const std::string& getMeetingIdHash() const;
        UserPtr getOwner() const;
        bool isOwner(const std::string& nicknameHash) const;

        void addParticipant(const UserPtr& user, const std::string& encryptedNickname);
        std::optional<std::string> removeParticipant(const std::string& nicknameHash);
        bool isParticipant(const std::string& nicknameHash) const;
        std::vector<ParticipantInfo> getParticipants() const;

        void addPendingJoinRequest(const PendingMeetingJoinRequestPtr& pendingRequest);
        PendingMeetingJoinRequestPtr removePendingJoinRequest(const std::string& requesterNicknameHash);
        PendingMeetingJoinRequestPtr getPendingJoinRequest(const std::string& requesterNicknameHash) const;
        std::vector<PendingMeetingJoinRequestPtr> getPendingJoinRequests() const;

        // Media sharing state within the meeting (by participant nickname hash).
        void addScreenSharer(const std::string& nicknameHash);
        void removeScreenSharer(const std::string& nicknameHash);
        std::vector<std::string> getScreenSharers() const;

        void addCameraSharer(const std::string& nicknameHash);
        void removeCameraSharer(const std::string& nicknameHash);
        std::vector<std::string> getCameraSharers() const;

        void clearMediaState();

    private:
        mutable std::mutex m_mutex;
        std::string m_meetingId;
        std::string m_meetingIdHash;
        UserPtr m_owner;
        std::unordered_map<std::string, ParticipantInfo> m_participants;
        std::unordered_map<std::string, PendingMeetingJoinRequestPtr> m_pendingJoinRequests;
        std::unordered_set<std::string> m_screenSharers;
        std::unordered_set<std::string> m_cameraSharers;
    };
}
