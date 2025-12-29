#pragma once 

#include <cstdint>
#include <unordered_map>
#include <string>

namespace calls {
    enum class PacketType : uint32_t {
        // only send
        AUTHORIZATION = 2,
        LOGOUT,
        GET_USER_INFO,

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

        // only receive
        AUTHORIZATION_SUCCESS,
        AUTHORIZATION_FAIL,
        AUTHORIZATION_SUCCESS,
        AUTHORIZATION_FAIL,
        GET_USER_INFO_SUCCESS,
        GET_USER_INFO_FAIL,
        CONNECTION_DOWN_WITH_USER,
        CONNECTION_RESTORED_WITH_USER
    };

    inline std::string packetTypeToString(PacketType type) {
        switch (type) {
            // only send
            case PacketType::AUTHORIZATION: return "AUTHORIZATION";
            case PacketType::LOGOUT: return "LOGOUT";
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

            // only receive
            case PacketType::AUTHORIZATION_SUCCESS: return "AUTHORIZATION_SUCCESS";
            case PacketType::AUTHORIZATION_FAIL: return "AUTHORIZATION_FAIL";
            case PacketType::GET_USER_INFO_SUCCESS: return "GET_USER_INFO_SUCCESS";
            case PacketType::GET_USER_INFO_FAIL: return "GET_USER_INFO_FAIL";
            case PacketType::CONNECTION_DOWN_WITH_USER: return "CONNECTION_DOWN_WITH_USER";
            case PacketType::CONNECTION_RESTORED_WITH_USER: return "CONNECTION_RESTORED_WITH_USER";

            default: return "UNKNOWN";
        }
    }
}
