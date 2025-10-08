#pragma once 

#include <cstdint>

namespace calls {

enum class PacketType : uint32_t {
	// only send
	AUTHORIZE,
	LOGOUT,
	GET_FRIEND_INFO,

	// send and receive
	CREATE_CALL,
	END_CALL,
	CALLING_END,
	CALL_ACCEPTED,
	CALL_DECLINED,

	// confirmations only receive
	AUTHORIZE_SUCCESS,
	AUTHORIZE_FAIL,
	LOGOUT_OK,
	GET_FRIEND_INFO_SUCCESS,
	GET_FRIEND_INFO_FAIL,
	CREATE_CALL_SUCCESS,
	CREATE_CALL_FAIL,
	END_CALL_OK,
	CALLING_END_OK,
	CALL_ACCEPTED_OK,
	CALL_DECLINED_OK,

	// special packets
	VOICE,
	PING,
	PING_SUCCESS
};

}
