#pragma once

#include <atomic>
#include <mutex>
#include <string>
#include <unordered_map>

#include "call.h"
#include "callStateManager.h"
#include "incomingCall.h"

namespace calls
{
    class ClientStateManager {
    public:
        ClientStateManager();
        ~ClientStateManager() = default;

        bool isAuthorized() const;
        bool isConnectionDown() const;
        bool isCallParticipantConnectionLost() const;
        bool isScreenSharing() const;
        bool isCameraSharing() const;
        bool isViewingRemoteScreen() const;
        bool isViewingRemoteCamera() const;

        void setAuthorized(bool value);
        void setConnectionDown(bool value);
        void setCallParticipantConnectionLost(bool value);
        void setScreenSharing(bool value);
        void setCameraSharing(bool value);
        void setViewingRemoteScreen(bool value);
        void setViewingRemoteCamera(bool value);

        const std::string& getMyNickname() const;
        void setMyNickname(const std::string& nickname);
        void clearMyNickname();

        const std::string& getMyToken() const;
        void setMyToken(const std::string& token);
        void clearMyToken();

        const CallStateManager& getCallStateManager() const;

        template <typename Rep, typename Period, typename Callable>
        void setOutgoingCall(const std::string& nickname,
            const std::chrono::duration<Rep, Period>& timeout,
            Callable&& onTimeout)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_callStateManager.setOutgoingCall(nickname, timeout, onTimeout);
        }

        void setActiveCall(const std::string& nickname,
            const CryptoPP::RSA::PublicKey& publicKey);

        void setActiveCallFromIncoming(const IncomingCall& incomingCall);

        void clearCallState();

        const std::unordered_map<std::string, IncomingCall>& getIncomingCalls() const;
        int getIncomingCallsCount() const;

        template <typename Rep, typename Period, typename Callable>
        void addIncomingCall(const std::string& nickname,
            const CryptoPP::RSA::PublicKey& publicKey,
            const CryptoPP::SecByteBlock& callKey,
            const std::chrono::duration<Rep, Period>& timeout,
            Callable&& onTimeout)
        {
            std::lock_guard<std::mutex> lock(m_mutex);

            IncomingCall incomingCall(nickname, publicKey, callKey, timeout, std::forward<Callable>(onTimeout));
            m_incomingCalls.emplace(nickname, std::move(incomingCall));
        }
        
        void removeIncomingCall(const std::string& nickname);

        void clearIncomingCalls();

        void reset();

    private:
        mutable std::mutex m_mutex;
        CallStateManager m_callStateManager;
        std::atomic_bool m_authorized;
        std::atomic_bool m_connectionDown;
        std::atomic_bool m_screenSharing;
        std::atomic_bool m_viewingRemoteScreen;
        std::atomic_bool m_cameraSharing;
        std::atomic_bool m_viewingRemoteCamera;
        std::string m_myNickname;
        std::string m_myToken;
        std::unordered_map<std::string, IncomingCall> m_incomingCalls;
    };
}