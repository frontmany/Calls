#pragma once 

#include <cstdint>
#include <unordered_map>
#include <string>

namespace calls {
enum class PacketType : uint32_t {
	// only send
	    AUTHORIZE,
	    LOGOUT,
	    GET_FRIEND_INFO,

	// send and receive
	START_SCREEN_SHARING,
	STOP_SCREEN_SHARING,
	SCREEN,
    START_CAMERA_SHARING,
    STOP_CAMERA_SHARING,
    CAMERA,
	START_CALLING,
	END_CALL,
	STOP_CALLING,
	CALL_ACCEPTED,
	CALL_DECLINED,
	END_CALL_OK,
	STOP_CALLING_OK,
	CALL_ACCEPTED_OK,
	CALL_DECLINED_OK,
	START_CALLING_OK,
	START_SCREEN_SHARING_OK,
	STOP_SCREEN_SHARING_OK,
    START_CAMERA_SHARING_OK,
    STOP_CAMERA_SHARING_OK,

	// confirmations only receive
    START_SCREEN_SHARING_FAIL,
    START_CAMERA_SHARING_FAIL,
	AUTHORIZE_SUCCESS,
	AUTHORIZE_FAIL,
	GET_FRIEND_INFO_SUCCESS,
	GET_FRIEND_INFO_FAIL,
	CALL_ACCEPTED_FAIL,
	START_CALLING_FAIL,
	LOGOUT_OK,

	// special packets
	VOICE,
	PING,
	PING_SUCCESS
};


inline std::string packetTypeToString(PacketType type) {
    static const std::unordered_map<PacketType, std::string> packetTypeMap = {
        {PacketType::AUTHORIZE, "AUTHORIZE"},
        {PacketType::LOGOUT, "LOGOUT"},
        {PacketType::GET_FRIEND_INFO, "GET_FRIEND_INFO"},
        {PacketType::START_SCREEN_SHARING, "START_SCREEN_SHARING"},
        {PacketType::STOP_SCREEN_SHARING, "STOP_SCREEN_SHARING"},
        {PacketType::SCREEN, "SCREEN"},
        {PacketType::START_CAMERA_SHARING, "START_CAMERA_SHARING"},
        {PacketType::STOP_CAMERA_SHARING, "STOP_CAMERA_SHARING"},
        {PacketType::CAMERA, "CAMERA"},
        {PacketType::START_CALLING, "START_CALLING"},
        {PacketType::END_CALL, "END_CALL"},
        {PacketType::STOP_CALLING, "STOP_CALLING"},
        {PacketType::CALL_ACCEPTED, "CALL_ACCEPTED"},
        {PacketType::CALL_DECLINED, "CALL_DECLINED"},
        {PacketType::END_CALL_OK, "END_CALL_OK"},
        {PacketType::STOP_CALLING_OK, "STOP_CALLING_OK"},
        {PacketType::CALL_ACCEPTED_OK, "CALL_ACCEPTED_OK"},
        {PacketType::CALL_DECLINED_OK, "CALL_DECLINED_OK"},
        {PacketType::START_CALLING_OK, "START_CALLING_OK"},
        {PacketType::START_SCREEN_SHARING_OK, "START_SCREEN_SHARING_OK"},
        {PacketType::STOP_SCREEN_SHARING_OK, "STOP_SCREEN_SHARING_OK"},
        {PacketType::START_SCREEN_SHARING_FAIL, "START_SCREEN_SHARING_FAIL"},
        {PacketType::START_CAMERA_SHARING_OK, "START_CAMERA_SHARING_OK"},
        {PacketType::STOP_CAMERA_SHARING_OK, "STOP_CAMERA_SHARING_OK"},
        {PacketType::START_CAMERA_SHARING_FAIL, "START_CAMERA_SHARING_FAIL"},
        {PacketType::AUTHORIZE_SUCCESS, "AUTHORIZE_SUCCESS"},
        {PacketType::AUTHORIZE_FAIL, "AUTHORIZE_FAIL"},
        {PacketType::GET_FRIEND_INFO_SUCCESS, "GET_FRIEND_INFO_SUCCESS"},
        {PacketType::GET_FRIEND_INFO_FAIL, "GET_FRIEND_INFO_FAIL"},
        {PacketType::CALL_ACCEPTED_FAIL, "CALL_ACCEPTED_FAIL"},
        {PacketType::START_CALLING_FAIL, "START_CALLING_FAIL"},
        {PacketType::LOGOUT_OK, "LOGOUT_OK"},
        {PacketType::VOICE, "VOICE"},
        {PacketType::PING, "PING"},
        {PacketType::PING_SUCCESS, "PING_SUCCESS"}
    };

    auto it = packetTypeMap.find(type);
    if (it != packetTypeMap.end()) {
        return it->second;
    }
    return "UNKNOWN";
}

}
