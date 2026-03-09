#pragma once 

#include <cstdint>
#include <unordered_map>
#include <string>

namespace core::constant
{
    enum class PacketType : uint32_t {
        // only send
        AUTHORIZATION,
        LOGOUT,
        RECONNECT,
        GET_USER_INFO,
        GET_METRICS,

        // send and receive
        CONFIRMATION,
        CALLING_BEGIN,
        CALLING_END,
        CALL_ACCEPT,
        CALL_DECLINE,
        CALL_END,
        SCREEN_SHARING_BEGIN,
        SCREEN_SHARING_END,
        SCREEN,
        CAMERA_SHARING_BEGIN,
        CAMERA_SHARING_END,
        CAMERA,
        VOICE,

        // meeting: send and receive
        MEETING_JOIN_REQUEST,
        MEETING_JOIN_CANCEL,
        MEETING_JOIN_ACCEPT,
        MEETING_JOIN_DECLINE,

        // meeting: send
        MEETING_CREATE,
        GET_MEETING_INFO,
        MEETING_LEAVE,
        MEETING_END,

        // meeting: receive
        MEETING_CREATE_RESULT,
        GET_MEETING_INFO_RESULT,
        MEETING_PARTICIPANT_JOINED,
        MEETING_PARTICIPANT_LEFT,
        MEETING_ENDED,

        // only receive
        AUTHORIZATION_RESULT,
        RECONNECT_RESULT,
        GET_USER_INFO_RESULT,
        GET_METRICS_RESULT,
        CONNECTION_DOWN_WITH_USER,
        CONNECTION_RESTORED_WITH_USER,
        USER_LOGOUT,
    };

    inline std::string packetTypeToString(PacketType type) {
        switch (type) {
            // only send
            case PacketType::AUTHORIZATION: return "AUTHORIZATION";
            case PacketType::LOGOUT: return "LOGOUT";
            case PacketType::RECONNECT: return "RECONNECT";
            case PacketType::GET_USER_INFO: return "GET_USER_INFO";

            // send and receive
            case PacketType::CONFIRMATION: return "CONFIRMATION";
            case PacketType::CALLING_BEGIN: return "CALLING_BEGIN";
            case PacketType::CALLING_END: return "CALLING_END";
            case PacketType::CALL_ACCEPT: return "CALL_ACCEPT";
            case PacketType::CALL_DECLINE: return "CALL_DECLINE";
            case PacketType::CALL_END: return "CALL_END";
            case PacketType::SCREEN_SHARING_BEGIN: return "SCREEN_SHARING_BEGIN";
            case PacketType::SCREEN_SHARING_END: return "SCREEN_SHARING_END";
            case PacketType::SCREEN: return "SCREEN";
            case PacketType::CAMERA_SHARING_BEGIN: return "CAMERA_SHARING_BEGIN";
            case PacketType::CAMERA_SHARING_END: return "CAMERA_SHARING_END";
            case PacketType::CAMERA: return "CAMERA";
            case PacketType::VOICE: return "VOICE";
            case PacketType::MEETING_CREATE: return "MEETING_CREATE";
            case PacketType::GET_MEETING_INFO: return "GET_MEETING_INFO";
            case PacketType::MEETING_JOIN_REQUEST: return "MEETING_JOIN_REQUEST";
            case PacketType::MEETING_JOIN_CANCEL: return "MEETING_JOIN_CANCEL";
            case PacketType::MEETING_JOIN_ACCEPT: return "MEETING_JOIN_ACCEPT";
            case PacketType::MEETING_JOIN_DECLINE: return "MEETING_JOIN_DECLINE";
            case PacketType::MEETING_LEAVE: return "MEETING_LEAVE";
            case PacketType::MEETING_END: return "MEETING_END";
            case PacketType::MEETING_CREATE_RESULT: return "MEETING_CREATE_RESULT";
            case PacketType::GET_MEETING_INFO_RESULT: return "GET_MEETING_INFO_RESULT";
            case PacketType::MEETING_ENDED: return "MEETING_ENDED";
            case PacketType::MEETING_PARTICIPANT_JOINED: return "MEETING_PARTICIPANT_JOINED";
            case PacketType::MEETING_PARTICIPANT_LEFT: return "MEETING_PARTICIPANT_LEFT";

            // only receive
            case PacketType::AUTHORIZATION_RESULT: return "AUTHORIZATION_RESULT";
            case PacketType::RECONNECT_RESULT: return "RECONNECT_RESULT";
            case PacketType::GET_USER_INFO_RESULT: return "GET_USER_INFO_RESULT";
            case PacketType::CONNECTION_DOWN_WITH_USER: return "CONNECTION_DOWN_WITH_USER";
            case PacketType::CONNECTION_RESTORED_WITH_USER: return "CONNECTION_RESTORED_WITH_USER";
            case PacketType::USER_LOGOUT: return "USER_LOGOUT";

            default: return "UNKNOWN";
        }
    }
}
