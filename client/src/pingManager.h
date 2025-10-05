#pragma once 

#include <functional>
#include <thread>
#include <memory>
#include <chrono>
#include <atomic>

namespace calls {
	class NetworkController;

	class PingManager {
	public:
		PingManager(std::shared_ptr<NetworkController> networkController, std::function<void()>&& onPingFail);
		~PingManager();
		void setPingSuccess();
		void start();
		void stop();

	private:
		void ping();
		void checkPing();

	private:
		std::shared_ptr<NetworkController> m_networkController;
		std::function<void()> m_onPingFail;
		std::thread m_pingThread;
		std::atomic_bool m_pingResult = false;
		std::atomic_bool m_running = false;
	};
}
