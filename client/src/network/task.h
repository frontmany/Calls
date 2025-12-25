#pragma once

#include <string>
#include <vector>
#include <functional>
#include <chrono>

#include "timer.h"
#include "packetTypes.h"
#include "networkController.h"

namespace calls {
	class Task {
	public:
		Task(std::shared_ptr<NetworkController> networkController, const std::string& uuid, std::vector<unsigned char> packet, PacketType type, std::function<void()>&& onAttemptFailed, std::function<void()>&& onAllAttemptsEnded, int maxAttempts);
		~Task();
		void retry();
		void setFinished();

	private:
		void send();

	private:
		std::shared_ptr<NetworkController> m_networkController;
		std::function<void()> m_onAllAttemptsFailed;
		std::function<void()> m_onAttemptFailed;
		Timer m_timer;
		PacketType m_type;
		std::string m_uuid; 
		std::vector<unsigned char> m_packet;
		int m_attempts = 0;
		int m_maxAttempts;
	};
}