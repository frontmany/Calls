#include "task.h"
#include "timer.h"

using namespace std::chrono_literals;

namespace calls 
{
	Task::Task(const std::string& uid,
		int maxAttempts,
		std::function<void()>&& attempt,
		std::function<void()>&& onFailed)
		: m_uid(uid),
		m_timer(Timer::create([this]() {retry(); })),
		m_maxAttempts(maxAttempts),
		m_attempts(0),
		m_attempt(attempt),
		m_onFailed(onFailed)
	{
	}

	Task::~Task() {
		setFinished();
	}

	void Task::start() {
		m_attempt();
		m_timer->start(2s);
	}

	void Task::setFinished() {
		m_timer->stop();
	}

	const std::string& Task::getUID() {
		return m_uid;
	}

	void Task::retry() {
		m_attempts++;

		if (m_attempts < m_maxAttempts) {
			start();
		}
		else {
			m_attempts = 0;
			m_onFailed();
		}
	}
}