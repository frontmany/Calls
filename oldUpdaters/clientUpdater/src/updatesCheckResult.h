#pragma once

namespace updater {

enum class UpdatesCheckResult : int {
	UPDATE_NOT_NEEDED,
	REQUIRED_UPDATE,
	POSSIBLE_UPDATE
};

}