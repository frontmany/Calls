#pragma once

namespace callifornia {
	namespace updater {

		enum class UpdateCheckResult : int {
			update_not_needed,
			required_update,
			possible_update
		};

	}
}