#pragma once

enum class CreateCallResult {
	WRONG_FRIEND_NICKNAME,
	TIMEOUT,
	UNAUTHORIZED,
	CALL_ACCEPTED,
	CALL_DECLINED
};

enum class AuthorizationResult {
	SUCCESS,
	FAIL,
	TIMEOUT
};