#pragma once

namespace calls {
	enum class ErrorCode {
		OK,
		TIMEOUT,
		TAKEN_NICKNAME,
		UNEXISTING_USER
	};
}
