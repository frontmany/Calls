#pragma once

namespace updater {

enum class CheckResult : int {
	ERROR = 1,
	REQUIRED_UPDATE,
	POSSIBLE_UPDATE
};

}