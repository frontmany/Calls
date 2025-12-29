#pragma once

#include <string>
#include <functional>
#include <chrono>
#include <memory>

namespace calls 
{
	class Timer;

	class Task {
	public:
		Task(const std::string& uid, int maxAttempts,
			std::function<void()>&& attempt,
			std::function<void()>&& onFailed
		);
		~Task();
		void start();
		void setFinished();
		const std::string& getUID();


	private:
		void retry();

	private:
		std::string m_uid;
		int m_attempts;
		int m_maxAttempts;
		std::shared_ptr<Timer> m_timer;
		std::function<void()> m_attempt;
		std::function<void()> m_onFailed;
	};
}