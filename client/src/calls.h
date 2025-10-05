#pragma once

#include <string>
#include <functional>

#include "callsClient.h"
#include "state.h"

namespace calls {
    // facade functions
    inline void init(
        const std::string& host,
        const std::string& port,
        std::function<void(Result)>&& authorizationResult,
        std::function<void(Result)>&& createCallResult,
        std::function<void(Result)>&& createGroupCallResult,
        std::function<void(Result)>&& joinGroupCallResult,
        std::function<void(const std::string&)>&& onJoinRequest,
        std::function<void(const std::string&)>&& onJoinRequestExpired,
        std::function<void(const std::string&)>&& onParticipantLeft,
        std::function<void(const std::string&)>&& onIncomingCall,
        std::function<void(const std::string&)>&& onIncomingCallExpired,
        std::function<void(const std::string&)>&& onCallingSomeoneWhoAlreadyCallingYou,
        std::function<void()>&& onRemoteUserEndedCall,
        std::function<void()>&& onNetworkError)
    {
        CallsClient::get().init(
            host,
            port,
            std::move(authorizationResult),
            std::move(createCallResult),
            std::move(createGroupCallResult),
            std::move(joinGroupCallResult),
            std::move(onJoinRequest),
            std::move(onJoinRequestExpired),
            std::move(onParticipantLeft),
            std::move(onIncomingCall),
            std::move(onIncomingCallExpired),
            std::move(onCallingSomeoneWhoAlreadyCallingYou),
            std::move(onRemoteUserEndedCall),
            std::move(onNetworkError));
    }

    inline void run()
    {
        CallsClient::get().run();
    }

    inline State getState()
    {
        return CallsClient::get().getStatus();
    }

    inline const std::string& getNickname()
    {
        return CallsClient::get().getNickname();
    }

    inline bool authorize(const std::string& nickname)
    {
        return CallsClient::get().authorize(nickname);
    }

    inline bool createCall(const std::string& friendNickname)
    {
        return CallsClient::get().createCall(friendNickname);
    }

    inline bool stopCalling()
    {
        return CallsClient::get().stopCalling();
    }

    inline bool acceptCall(const std::string& friendNickname)
    {
        return CallsClient::get().acceptIncomingCall(friendNickname);
    }

    inline bool declineCall(const std::string& friendNickname)
    {
        return CallsClient::get().declineIncomingCall(friendNickname);
    }

    inline bool declineAllCalls()
    {
        return CallsClient::get().declineAllIncomingCalls();
    }

    inline bool endCall()
    {
        return CallsClient::get().endCall();
    }

    inline int getIncomingCallsCount() {
        return CallsClient::get().getIncomingCallsCount();
    }

    inline void refreshAudioDevices() {
        CallsClient::get().refreshAudioDevices();
    }

    inline void mute(bool isMute)
    {
        CallsClient::get().mute(isMute);
    }

    inline bool isMuted()
    {
        return CallsClient::get().isMuted();
    }

    inline void setInputVolume(int volume)
    {
        CallsClient::get().setInputVolume(volume);
    }

    inline void setOutputVolume(int volume)
    {
        CallsClient::get().setOutputVolume(volume);
    }

    inline int getInputVolume()
    {
        return CallsClient::get().getInputVolume();
    }

    inline int getOutputVolume()
    {
        return CallsClient::get().getOutputVolume();
    }

    inline void logout()
    {
        CallsClient::get().logout();
    }

    inline void stop()
    {
        CallsClient::get().stop();
    }

    inline bool isRunning()
    {
        return CallsClient::get().isRunning();
    }

    inline const std::string& getNicknameWhomCalling() {
        return CallsClient::get().getNicknameWhomCalling();
    }

    inline const std::string& getNicknameInCallWith() {
        return CallsClient::get().getNicknameInCallWith();
    }


    inline bool createGroupCall(const std::string& groupCallName) {
        return CallsClient::get().createGroupCall(groupCallName);
    }

    inline bool joinGroupCall(const std::string& groupCallName) {
        return CallsClient::get().joinGroupCall(groupCallName);
    }

    inline bool endGroupCall(const std::string& groupCallName) {
        return CallsClient::get().endGroupCall(groupCallName);
    }

    inline bool leaveGroupCall(const std::string& groupCallName) {
        return CallsClient::get().leaveGroupCall(groupCallName);
    }

    inline bool allowJoin(const std::string& friendNickname) {
        return CallsClient::get().allowJoin(friendNickname);
    }

    inline bool declineJoin(const std::string& friendNickname) {
        return CallsClient::get().declineJoin(friendNickname);
    }
}