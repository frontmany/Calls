#pragma once

enum class PacketType : int {
	CHECK_UPDATES,
	UPDATE_ACCEPT,
	UPDATE_DECLINE
};