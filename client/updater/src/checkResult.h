#pragma once

namespace updater
{
enum class CheckResult : int {
	UPDATE_NOT_NEEDED,
	REQUIRED_UPDATE,
	POSSIBLE_UPDATE
};
}
