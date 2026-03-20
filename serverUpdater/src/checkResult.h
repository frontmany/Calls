#pragma once

namespace serverUpdater
{
enum class CheckResult : int {
	UPDATE_NOT_NEEDED,
	REQUIRED_UPDATE,
	POSSIBLE_UPDATE,
    UPDATE_CHECK_FAILED
};
}