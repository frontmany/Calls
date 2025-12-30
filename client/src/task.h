#pragma once

#include <string>
#include <functional>
#include <chrono>
#include <memory>

#include "utilities/timer.h"
#include "utilities/logger.h"

namespace calls
{
	template <typename Rep, typename Period>
	class Task {
	public:
		Task(const std::string& uid,
			const std::chrono::duration<Rep, Period>& period,
			int maxAttempts,
			std::function<void()>&& attempt,
			std::function<void()>&& onFinishedSuccessfully,
			std::function<void()>&& onFailed)
			: m_uid(uid),
			m_timer(),
			m_period(period),
			m_maxAttempts(maxAttempts),
			m_attempt(attempt),
			m_onFinishedSuccessfully(onFinishedSuccessfully),
			m_onFailed(onFailed)
		{
		}

		~Task() {
			m_timer.stop();
		}

		void start() {
			m_attempt();
			m_attemptsCount++;

			m_timer.start(m_period, [this]() {
				try {
					if (m_attemptsCount == m_maxAttempts) {
						m_timer.stop();
						m_onFailed();

						return;
					}

					m_attempt();
					m_attemptsCount++;
				}
				catch (const std::exception& e) {
					log::LOG_ERROR("Exception during task attempt {}", e.what());
				}
			});
		}

		void complete() {
			m_timer.stop();
			m_onFinishedSuccessfully();
		}

		void fail() {
			m_timer.stop();
			m_onFailed();
		}

		void cancel() {
			m_timer.stop();
		}

		const std::string& getUID() {
			return m_uid;
		}

	private:
		std::string m_uid;
		int m_attemptsCount = 0;
		int m_maxAttempts;
		const std::chrono::duration<Rep, Period>& m_period;
		utilities::tic::RapidTimer m_timer;
		std::function<void()> m_attempt;
		std::function<void()> m_onFinishedSuccessfully;
		std::function<void()> m_onFailed;
	};
}