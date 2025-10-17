#pragma once 

#include <cstdint>

namespace calls {

enum class PacketType : uint32_t {
	// only send
	AUTHORIZE,
	LOGOUT,
	GET_FRIEND_INFO,

	// send and receive
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

	// confirmations only receive
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

}
