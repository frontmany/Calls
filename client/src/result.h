#pragma once

namespace calls {

enum class Result {
	WRONG_FRIEND_NICKNAME,
	TIMEOUT,
	UNAUTHORIZED,
	CALL_ACCEPTED,
	CALL_DECLINED,
	SUCCESS,
	FAIL
};
}