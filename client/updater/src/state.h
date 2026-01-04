#pragma once

namespace updater
{
	enum class State {
		DISCONNECTED,
		AWAITING_SERVER_RESPONSE,
		AWAITING_UPDATES_CHECK,
		AWAITING_START_UPDATE
	};
}