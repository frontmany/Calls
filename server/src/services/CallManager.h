#pragma once

#include <unordered_set>
#include <memory>
#include <mutex>
#include <string>
#include "ICallManager.h"
#include "call.h"
#include "pendingCall.h"
#include "user.h"

namespace server
{
    namespace services
    {
        // Менеджер для управления звонками
        class CallManager : public ICallManager {
        public:
            CallManager() = default;
            ~CallManager() = default;

            bool isUserInCall(const std::string& nicknameHash) override {
                std::lock_guard<std::mutex> lock(m_mutex);
                // Ищем звонок, в котором участвует пользователь
                for (const auto& call : m_calls) {
                    if (call->getInitiator()->getNicknameHash() == nicknameHash ||
                        call->getResponder()->getNicknameHash() == nicknameHash) {
                        return true;
                    }
                }
                return false;
            }

            UserPtr getCallPartner(const std::string& nicknameHash) override {
                std::lock_guard<std::mutex> lock(m_mutex);
                for (const auto& call : m_calls) {
                    if (call->getInitiator()->getNicknameHash() == nicknameHash) {
                        return call->getResponder();
                    }
                    if (call->getResponder()->getNicknameHash() == nicknameHash) {
                        return call->getInitiator();
                    }
                }
                return nullptr;
            }

            std::shared_ptr<Call> createCall(UserPtr user1, UserPtr user2) override {
                if (!user1 || !user2) {
                    return nullptr;
                }

                std::lock_guard<std::mutex> lock(m_mutex);
                auto call = std::make_shared<Call>(user1, user2);
                m_calls.insert(call);
                return call;
            }

            void endCall(const std::string& user1Nickname, const std::string& user2Nickname) override {
                std::lock_guard<std::mutex> lock(m_mutex);
                
                auto it = m_calls.begin();
                while (it != m_calls.end()) {
                    const auto& call = *it;
                    auto initiatorHash = call->getInitiator()->getNicknameHash();
                    auto responderHash = call->getResponder()->getNicknameHash();
                    
                    if ((initiatorHash == user1Nickname && responderHash == user2Nickname) ||
                        (initiatorHash == user2Nickname && responderHash == user1Nickname)) {
                        it = m_calls.erase(it);
                        return;
                    } else {
                        ++it;
                    }
                }
            }

            std::shared_ptr<Call> getCall(const std::string& nicknameHash) override {
                std::lock_guard<std::mutex> lock(m_mutex);
                for (const auto& call : m_calls) {
                    if (call->getInitiator()->getNicknameHash() == nicknameHash ||
                        call->getResponder()->getNicknameHash() == nicknameHash) {
                        return call;
                    }
                }
                return nullptr;
            }

            void addPendingCall(std::shared_ptr<PendingCall> pendingCall) override {
                if (!pendingCall) return;
                std::lock_guard<std::mutex> lock(m_mutex);
                m_pendingCalls.insert(pendingCall);
            }

            void removePendingCall(std::shared_ptr<PendingCall> pendingCall) override {
                if (!pendingCall) return;
                std::lock_guard<std::mutex> lock(m_mutex);
                m_pendingCalls.erase(pendingCall);
            }

            bool hasPendingCall(std::shared_ptr<PendingCall> pendingCall) const override {
                if (!pendingCall) return false;
                std::lock_guard<std::mutex> lock(m_mutex);
                return m_pendingCalls.find(pendingCall) != m_pendingCalls.end();
            }

            // Получить прямой доступ (для обратной совместимости)
            std::unordered_set<std::shared_ptr<Call>>& getCalls() {
                return m_calls;
            }

            std::unordered_set<std::shared_ptr<PendingCall>>& getPendingCalls() {
                return m_pendingCalls;
            }

            std::mutex& getMutex() {
                return m_mutex;
            }

        private:
            mutable std::mutex m_mutex;
            std::unordered_set<std::shared_ptr<Call>> m_calls;
            std::unordered_set<std::shared_ptr<PendingCall>> m_pendingCalls;
        };
    }
}
