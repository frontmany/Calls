#pragma once

#include <atomic>
#include <chrono>
#include <mutex>
#include <string>
#include <unordered_map>
#include <functional>

#include "model/call.h"
#include "model/incomingCall.h"
#include "model/outgoingCall.h"
#include "media/mediaState.h"
#include "media/mediaType.h"

namespace core
{
    class ClientStateManager {
    public:
        ClientStateManager();
        ~ClientStateManager() = default;

        bool isAuthorized() const;
        bool isConnectionDown() const;
        bool isIncomingCalls() const;
        bool isViewingRemoteScreen() const;
        bool isViewingRemoteCamera() const;
        bool isOutgoingCall() const;
        bool isActiveCall() const;
        bool isCallParticipantConnectionDown() const;

        void setAuthorized(bool value);
        void setConnectionDown(bool value);
        void setCallParticipantConnectionDown(bool value);
        void setLastReconnectSuccessTime();
        bool isInReconnectGracePeriod() const;
        void setMediaState(media::MediaType type, media::MediaState state);
        void setViewingRemoteScreen(bool value);
        void setViewingRemoteCamera(bool value);

        const std::string& getMyNickname() const;
        void setMyNickname(const std::string& nickname);
        void clearMyNickname();

        const std::string& getMyToken() const;
        void setMyToken(const std::string& token);
        void clearMyToken();

        const media::MediaState getMediaState(media::MediaType type) const;
        const OutgoingCall& getOutgoingCall() const;
        const Call& getActiveCall() const;

        template <typename Rep, typename Period>
        void setOutgoingCall(const std::string& nickname,
            const std::chrono::duration<Rep, Period>& timeout,
            std::function<void()> onTimeout)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_callStateManager.setOutgoingCall(nickname, timeout, std::move(onTimeout));
        }

        void setActiveCall(const std::string& nickname,
            const CryptoPP::RSA::PublicKey& publicKey, const CryptoPP::SecByteBlock& callKey);

        void clearCallState();

        const std::unordered_map<std::string, IncomingCall>& getIncomingCalls() const;
        int getIncomingCallsCount() const;

        template <typename Rep, typename Period>
        void addIncomingCall(const std::string& nickname,
            const CryptoPP::RSA::PublicKey& publicKey,
            const CryptoPP::SecByteBlock& callKey,
            const std::chrono::duration<Rep, Period>& timeout,
            std::function<void()> onTimeout)
        {
            std::lock_guard<std::mutex> lock(m_mutex);

            IncomingCall incomingCall(nickname, publicKey, callKey, timeout, std::move(onTimeout));
            m_incomingCalls.emplace(nickname, std::move(incomingCall));
        }
        
        void removeIncomingCall(const std::string& nickname);
        bool tryRemoveIncomingCall(const std::string& nickname);

        void clearIncomingCalls();

        void reset();

    private:
        mutable std::mutex m_mutex;
        std::map<media::MediaType, media::MediaState> m_mediaState;
        std::atomic_bool m_authorized;
        std::atomic_bool m_connectionDown;
        std::atomic_bool m_viewingRemoteScreen;
        std::atomic_bool m_viewingRemoteCamera;
        std::string m_myNickname;
        std::string m_myToken;

        std::optional<Call> m_activeCall;
        std::optional<OutgoingCall> m_outgoingCall;
        std::unordered_map<std::string, IncomingCall> m_incomingCalls;

        std::chrono::steady_clock::time_point m_lastReconnectSuccessTime{};
    };
}