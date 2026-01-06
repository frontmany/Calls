#pragma once

namespace updater
{
	enum class UpdateCheckResult : uint32_t {
		update_not_needed,
		required_update,
		possible_update
	};
}