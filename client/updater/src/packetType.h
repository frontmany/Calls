#pragma once

#include <cstdint>

namespace updater
{
	enum class PacketType : uint32_t {
		// only send
		UPDATE_CHECK,
		UPDATE_ACCEPT,

		// only receive
		UPDATE_METADATA,
		UPDATE_RESULT
	};
}