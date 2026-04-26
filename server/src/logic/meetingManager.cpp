#include "meetingManager.h"

namespace server::logic
{
    MeetingPtr MeetingManager::createMeeting(const std::string& meetingId, const std::string& meetingIdHash, const UserPtr& owner)
    {
        if (!owner) {
            return nullptr;
        }

        std::lock_guard<std::mutex> lock(m_mutex);
        auto meeting = std::make_shared<Meeting>(meetingId, meetingIdHash, owner);
        m_meetingsByIdHash[meetingIdHash] = meeting;
        return meeting;
    }

    MeetingPtr MeetingManager::findByIdHash(const std::string& meetingIdHash) const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_meetingsByIdHash.find(meetingIdHash);
        return it == m_meetingsByIdHash.end() ? nullptr : it->second;
    }

    void MeetingManager::endMeeting(const MeetingPtr& meeting)
    {
        if (!meeting) {
            return;
        }

        std::lock_guard<std::mutex> lock(m_mutex);
        m_meetingsByIdHash.erase(meeting->getMeetingIdHash());
    }

    void MeetingManager::addPendingJoinRequest(const PendingMeetingJoinRequestPtr& pendingJoinRequest)
    {
        if (!pendingJoinRequest) {
            return;
        }

        std::lock_guard<std::mutex> lock(m_mutex);
        m_pendingJoinRequests.insert(pendingJoinRequest);
    }

    void MeetingManager::removePendingJoinRequest(const PendingMeetingJoinRequestPtr& pendingJoinRequest)
    {
        if (!pendingJoinRequest) {
            return;
        }

        std::lock_guard<std::mutex> lock(m_mutex);
        m_pendingJoinRequests.erase(pendingJoinRequest);
    }

    size_t MeetingManager::getActiveMeetingsCount() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_meetingsByIdHash.size();
    }

    size_t MeetingManager::getPendingJoinRequestsCount() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_pendingJoinRequests.size();
    }
}
