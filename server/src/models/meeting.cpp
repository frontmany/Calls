#include "meeting.h"

#include "pendingMeetingJoinRequest.h"
#include "user.h"

namespace server
{
    Meeting::Meeting(const std::string& meetingId, const std::string& meetingIdHash, const UserPtr& owner)
        : m_meetingId(meetingId)
        , m_meetingIdHash(meetingIdHash)
        , m_owner(owner)
    {
    }

    const std::string& Meeting::getMeetingId() const
    {
        return m_meetingId;
    }

    const std::string& Meeting::getMeetingIdHash() const
    {
        return m_meetingIdHash;
    }

    UserPtr Meeting::getOwner() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_owner;
    }

    bool Meeting::isOwner(const std::string& nicknameHash) const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_owner && m_owner->getNicknameHash() == nicknameHash;
    }

    void Meeting::addParticipant(const UserPtr& user, const std::string& encryptedNickname)
    {
        if (!user) {
            return;
        }

        std::lock_guard<std::mutex> lock(m_mutex);
        m_participants[user->getNicknameHash()] = ParticipantInfo{ user, encryptedNickname };
    }

    std::optional<std::string> Meeting::removeParticipant(const std::string& nicknameHash)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_participants.find(nicknameHash);
        if (it == m_participants.end()) {
            return std::nullopt;
        }

        std::string encryptedNickname = it->second.encryptedNickname;
        m_participants.erase(it);
        return encryptedNickname;
    }

    bool Meeting::isParticipant(const std::string& nicknameHash) const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_participants.contains(nicknameHash);
    }

    std::vector<Meeting::ParticipantInfo> Meeting::getParticipants() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::vector<ParticipantInfo> participants;
        participants.reserve(m_participants.size());
        for (const auto& [_, participant] : m_participants) {
            participants.push_back(participant);
        }
        return participants;
    }

    void Meeting::addPendingJoinRequest(const PendingMeetingJoinRequestPtr& pendingRequest)
    {
        if (!pendingRequest) {
            return;
        }

        auto requester = pendingRequest->getRequester();
        if (!requester) {
            return;
        }

        std::lock_guard<std::mutex> lock(m_mutex);
        m_pendingJoinRequests[requester->getNicknameHash()] = pendingRequest;
    }

    PendingMeetingJoinRequestPtr Meeting::removePendingJoinRequest(const std::string& requesterNicknameHash)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_pendingJoinRequests.find(requesterNicknameHash);
        if (it == m_pendingJoinRequests.end()) {
            return nullptr;
        }

        auto pending = it->second;
        m_pendingJoinRequests.erase(it);
        return pending;
    }

    PendingMeetingJoinRequestPtr Meeting::getPendingJoinRequest(const std::string& requesterNicknameHash) const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_pendingJoinRequests.find(requesterNicknameHash);
        return it == m_pendingJoinRequests.end() ? nullptr : it->second;
    }

    std::vector<PendingMeetingJoinRequestPtr> Meeting::getPendingJoinRequests() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::vector<PendingMeetingJoinRequestPtr> requests;
        requests.reserve(m_pendingJoinRequests.size());
        for (const auto& [_, request] : m_pendingJoinRequests) {
            requests.push_back(request);
        }
        return requests;
    }

    void Meeting::addScreenSharer(const std::string& nicknameHash)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_screenSharers.insert(nicknameHash);
    }

    void Meeting::removeScreenSharer(const std::string& nicknameHash)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_screenSharers.erase(nicknameHash);
    }

    std::vector<std::string> Meeting::getScreenSharers() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return std::vector<std::string>(m_screenSharers.begin(), m_screenSharers.end());
    }

    void Meeting::addCameraSharer(const std::string& nicknameHash)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_cameraSharers.insert(nicknameHash);
    }

    void Meeting::removeCameraSharer(const std::string& nicknameHash)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_cameraSharers.erase(nicknameHash);
    }

    std::vector<std::string> Meeting::getCameraSharers() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return std::vector<std::string>(m_cameraSharers.begin(), m_cameraSharers.end());
    }

    void Meeting::addMutedParticipant(const std::string& nicknameHash)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_mutedParticipants.insert(nicknameHash);
    }

    void Meeting::removeMutedParticipant(const std::string& nicknameHash)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_mutedParticipants.erase(nicknameHash);
    }

    std::vector<std::string> Meeting::getMutedParticipants() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return std::vector<std::string>(m_mutedParticipants.begin(), m_mutedParticipants.end());
    }

    void Meeting::clearMediaState()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_screenSharers.clear();
        m_cameraSharers.clear();
        m_mutedParticipants.clear();
    }

    void Meeting::setCameraSubscriptionLayer(const std::string& receiverHash, const std::string& senderHash, uint8_t maxLayer)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_cameraSubscriptions[receiverHash][senderHash] = maxLayer;
    }

    uint8_t Meeting::getCameraSubscriptionLayer(const std::string& receiverHash, const std::string& senderHash) const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto receiverIt = m_cameraSubscriptions.find(receiverHash);
        if (receiverIt == m_cameraSubscriptions.end()) {
            return 2;
        }
        auto senderIt = receiverIt->second.find(senderHash);
        if (senderIt == receiverIt->second.end()) {
            return 2;
        }
        return senderIt->second;
    }

    uint8_t Meeting::getRequiredSenderLayer(const std::string& senderHash) const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        bool found = false;
        uint8_t required = 0;
        for (const auto& [receiverHash, perSender] : m_cameraSubscriptions) {
            (void)receiverHash;
            auto it = perSender.find(senderHash);
            if (it == perSender.end()) {
                continue;
            }
            found = true;
            required = std::max(required, it->second);
        }
        return found ? required : 2;
    }
}
