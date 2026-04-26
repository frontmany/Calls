#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <cstddef>
#include <unordered_map>
#include <unordered_set>

#include "models/meeting.h"
#include "models/pendingMeetingJoinRequest.h"

namespace server::logic
{
    class MeetingManager {
    public:
        MeetingManager() = default;
        ~MeetingManager() = default;

        MeetingPtr createMeeting(const std::string& meetingId, const std::string& meetingIdHash, const UserPtr& owner);
        MeetingPtr findByIdHash(const std::string& meetingIdHash) const;
        void endMeeting(const MeetingPtr& meeting);

        void addPendingJoinRequest(const PendingMeetingJoinRequestPtr& pendingJoinRequest);
        void removePendingJoinRequest(const PendingMeetingJoinRequestPtr& pendingJoinRequest);
        size_t getActiveMeetingsCount() const;
        size_t getPendingJoinRequestsCount() const;

    private:
        mutable std::mutex m_mutex;
        std::unordered_map<std::string, MeetingPtr> m_meetingsByIdHash;
        std::unordered_set<PendingMeetingJoinRequestPtr> m_pendingJoinRequests;
    };
}
