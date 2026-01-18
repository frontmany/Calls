#pragma once

#include <string>
#include <functional>
#include <chrono>
#include <optional>
#include <atomic>
#include <memory>

#include "ticTimer.h"
#include "utilities/logger.h"
#include "json.hpp"

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
		m_period(period),
		m_timer(std::make_shared<tic::RapidTimer>()),
		m_state(std::make_shared<State>(maxAttempts, std::move(attempt),
			std::move(onFinishedSuccessfully), std::move(onFailed)))
	{
	}

	Task(const Task&) = delete;
	Task& operator=(const Task&) = delete;
	Task(Task&&) = default;
	Task& operator=(Task&&) = default;

	~Task() {
		deactivate();
		stopTimer();
	}

	void start() {
		m_state->attempt();
		m_state->attemptsCount.fetch_add(1);

		m_timer->start(m_period, std::bind(&Task::onTimerTick,
			std::weak_ptr<State>(m_state),
			std::weak_ptr<tic::RapidTimer>(m_timer)));
	}

	void complete(std::optional<nlohmann::json> completionContext = std::nullopt) {
		deactivate();
		stopTimer();
		if (m_state->onFinishedSuccessfully) {
			m_state->onFinishedSuccessfully(completionContext);
		}
	}

	void fail(std::optional<nlohmann::json> failureContext = std::nullopt) {
		deactivate();
		stopTimer();
		if (m_state->onFailed) {
			m_state->onFailed(failureContext);
		}
	}

	void cancel() {
		deactivate();
		stopTimer();
	}

	const std::string& getUID() {
		return m_uid;
	}

private:
	struct State {
		explicit State(int maxAttempts,
			std::function<void()>&& attempt,
			std::function<void(std::optional<nlohmann::json>)>&& onFinishedSuccessfully,
			std::function<void(std::optional<nlohmann::json>)>&& onFailed)
			: maxAttempts(maxAttempts)
			, attempt(std::move(attempt))
			, onFinishedSuccessfully(std::move(onFinishedSuccessfully))
			, onFailed(std::move(onFailed))
		{
		}

		std::atomic<bool> active = true;
		std::atomic<int> attemptsCount = 0;
		int maxAttempts;
		std::function<void()> attempt;
		std::function<void(std::optional<nlohmann::json>)> onFinishedSuccessfully;
		std::function<void(std::optional<nlohmann::json>)> onFailed;
	};

	static void onTimerTick(std::weak_ptr<State> stateWeak,
		std::weak_ptr<tic::RapidTimer> timerWeak) {
		auto state = stateWeak.lock();
		if (!state) {
			return;
		}

		if (!state->active.load()) {
			stopTimer(timerWeak);
			return;
		}

		try {
			if (state->attemptsCount.load() == state->maxAttempts) {
				stopTimer(timerWeak);
				state->active.store(false);
				if (state->onFailed) {
					state->onFailed(std::nullopt);
				}

				return;
			}

			state->attempt();
			state->attemptsCount.fetch_add(1);
		}
		catch (const std::exception& e) {
			LOG_ERROR("Exception during task attempt {}", e.what());
		}
	}

	static void stopTimer(const std::weak_ptr<tic::RapidTimer>& timerWeak) {
		if (auto timer = timerWeak.lock()) {
			timer->stop();
		}
	}

	void stopTimer() {
		if (m_timer) {
			m_timer->stop();
		}
	}

	void deactivate() {
		if (m_state) {
			m_state->active.store(false);
		}
	}

	std::string m_uid;
	std::chrono::duration<Rep, Period> m_period;
	std::shared_ptr<tic::RapidTimer> m_timer;
	std::shared_ptr<State> m_state;
};
