#pragma once

#include <string>
#include <functional>
#include <chrono>
#include <optional>

#include "ticTimer.h"
#include "utilities/logger.h"
#include "json.hpp"

namespace core 
{
	template <typename Rep, typename Period>
	class Task {
	public:
		Task(const std::string& uid,
			const std::chrono::duration<Rep, Period>& period,
			int maxAttempts,
			std::function<void()>&& attempt,
			std::function<void(std::optional<nlohmann::json>)>&& onFinishedSuccessfully,
			std::function<void(std::optional<nlohmann::json>)>&& onFailed)
			: m_uid(uid),
			m_timer(),
			m_period(period),
			m_maxAttempts(maxAttempts),
			m_attempt(attempt),
			m_onFinishedSuccessfully(onFinishedSuccessfully),
			m_onFailed(onFailed)
		{
		}

		Task(const Task&) = delete;
		Task& operator=(const Task&) = delete;
		Task(Task&&) = default;
		Task& operator=(Task&&) = default;

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
						m_onFailed(std::nullopt);

						return;
					}

					m_attempt();
					m_attemptsCount++;
				}
				catch (const std::exception& e) {
					LOG_ERROR("Exception during task attempt {}", e.what());
				}
				});
		}

		void complete(std::optional<nlohmann::json> completionContext = std::nullopt) {
			m_timer.stop();
			m_onFinishedSuccessfully(completionContext);
		}

		void fail(std::optional<nlohmann::json> failureContext = std::nullopt) {
			m_timer.stop();
			m_onFailed(failureContext);
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
		std::chrono::duration<Rep, Period> m_period;
		tic::RapidTimer m_timer;
		std::function<void()> m_attempt;
		std::function<void(std::optional<nlohmann::json>)> m_onFinishedSuccessfully;
		std::function<void(std::optional<nlohmann::json>)> m_onFailed;
	};
}