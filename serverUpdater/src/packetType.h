#pragma once

namespace serverUpdater
{
enum class PacketType : int {
	// only receive
	UPDATE_CHECK,
	UPDATE_ACCEPT,

	// only send
	UPDATE_METADATA,
	UPDATE_RESULT,
};
}