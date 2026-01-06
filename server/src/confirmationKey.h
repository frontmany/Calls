#pragma once

#include <string>
#include <functional>

struct ConfirmationKey {
	std::string uid;
	std::string nicknameHash;
	std::string token;

	bool operator==(const ConfirmationKey& other) const
	{
		return uid == other.uid && nicknameHash == other.nicknameHash && token == other.token;
	}
};

namespace std
{
	template<>
	struct hash<ConfirmationKey>
	{
		std::size_t operator()(const ConfirmationKey& key) const
		{
			std::size_t h1 = std::hash<std::string>{}(key.nicknameHash);
			std::size_t h2 = std::hash<std::string>{}(key.token);
			std::size_t h3 = std::hash<std::string>{}(key.uid);
			
			h1 ^= h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2);
			h1 ^= h3 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2);
			return h1;
		}
	};
}