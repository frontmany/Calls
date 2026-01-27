#pragma once

#include "ICallService.h"
#include "clientStateManager.h"
#include "keyManager.h"
#include "userOperationManager.h"
#include "taskManager.h"
#include "packetFactory.h"
#include "packetType.h"
#include "errorCode.h"
#include "eventListener.h"
#include "network/networkController.h"
#include "audio/audioEngine.h"
#include "json.hpp"
#include <functional>
#include <memory>
#include <optional>

namespace core
{
    namespace services
    {
        // Сервис для управления звонками
        class CallService : public ICallService {
        public:
            CallService(
                ClientStateManager& stateManager,
                KeyManager& keyManager,
                UserOperationManager& operationManager,
                TaskManager<long long, std::milli>& taskManager,
                core::network::NetworkController& networkController,
                core::audio::AudioEngine& audioEngine,
                std::shared_ptr<EventListener> eventListener
            );

            std::error_code startOutgoingCall(const std::string& nickname) override;
            std::error_code stopOutgoingCall() override;
            std::error_code acceptCall(const std::string& nickname) override;
            std::error_code declineCall(const std::string& nickname) override;
            std::error_code endCall() override;

            // Callbacks для обработки результатов операций
            void onRequestUserInfoCompleted(const std::string& userNickname, std::optional<nlohmann::json> completionContext);
            void onRequestUserInfoFailed(const std::string& userNickname, std::optional<nlohmann::json> failureContext);
            void onStartOutgoingCallCompleted(const std::string& userNickname, std::optional<nlohmann::json> completionContext);
            void onStartOutgoingCallFailed(const std::string& userNickname, std::optional<nlohmann::json> failureContext);
            void onOutgoingCallTimeout();
            void onStopOutgoingCallCompleted(const std::string& nickname, std::optional<nlohmann::json> completionContext);
            void onStopOutgoingCallFailed(const std::string& nickname, std::optional<nlohmann::json> failureContext);
            void onDeclineIncomingCallCompleted(const std::string& nickname, std::optional<nlohmann::json> completionContext);
            void onDeclineIncomingCallFailed(std::optional<nlohmann::json> failureContext);
            void onAcceptCallStopOutgoingCompleted(const std::string& userNickname, std::optional<nlohmann::json> completionContext);
            void onAcceptCallStopOutgoingFailed(const std::string& userNickname, std::optional<nlohmann::json> failureContext);
            void onAcceptCallEndActiveCompleted(const std::string& userNickname, std::optional<nlohmann::json> completionContext);
            void onAcceptCallEndActiveFailed(const std::string& userNickname, std::optional<nlohmann::json> failureContext);
            void onAcceptCallCompleted(const std::string& userNickname, std::optional<nlohmann::json> completionContext);
            void onAcceptCallFailed(const std::string& userNickname, std::optional<nlohmann::json> failureContext);
            void onAcceptCallFailedAfterEndCall(const std::string& userNickname, std::optional<nlohmann::json> failureContext);
            void onDeclineCallCompleted(const std::string& userNickname, std::optional<nlohmann::json> completionContext);
            void onDeclineCallFailed(const std::string& userNickname, std::optional<nlohmann::json> failureContext);
            void onEndCallCompleted(const std::string& nickname, std::optional<nlohmann::json> completionContext);
            void onEndCallFailed(const std::string& nickname, std::optional<nlohmann::json> failureContext);

        private:
            void createAndStartTask(
                const std::string& uid,
                const std::vector<unsigned char>& packet,
                PacketType packetType,
                std::function<void(std::optional<nlohmann::json>)> onCompletion,
                std::function<void(std::optional<nlohmann::json>)> onFailure);

        private:
            ClientStateManager& m_stateManager;
            KeyManager& m_keyManager;
            UserOperationManager& m_operationManager;
            TaskManager<long long, std::milli>& m_taskManager;
            core::network::NetworkController& m_networkController;
            core::audio::AudioEngine& m_audioEngine;
            std::shared_ptr<EventListener> m_eventListener;
        };
    }
}
