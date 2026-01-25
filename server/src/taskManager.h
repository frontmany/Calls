#pragma once

#include "task.h"
#include "utilities/crypto.h"

#include <string>
#include <unordered_map>
#include <mutex>
#include <functional>

using namespace server::utilities;

template <typename Rep, typename Period>
class TaskManager 
{
public:
	TaskManager() = default;
	~TaskManager()= default;

	void createTask(const std::string& uid,
		const std::chrono::duration<Rep, Period>& period,
		int maxAttempts,
		std::function<void()>&& attempt,
		std::function<void(std::optional<nlohmann::json>)>&& onFinishedSuccessfully,
		std::function<void(std::optional<nlohmann::json>)>&& onFailed)
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		Task task = Task(uid, period, maxAttempts, std::move(attempt), std::move(onFinishedSuccessfully), std::move(onFailed),
			[this, uid]() { removeTask(uid); });
		m_tasks.emplace(uid, std::move(task));
	}

	void startTask(const std::string& uid) {
		std::lock_guard<std::mutex> lock(m_mutex);

		if (m_tasks.contains(uid)) {
			auto& task = m_tasks.at(uid);
			task.start();
		}
	}

	void completeTask(const std::string& uid, std::optional<nlohmann::json> completionContext = std::nullopt) {
		std::lock_guard<std::mutex> lock(m_mutex);

		auto it = m_tasks.find(uid);
		if (it != m_tasks.end()) {
			it->second.complete(completionContext);
			m_tasks.erase(it);
		}
	}

	void failTask(const std::string& uid, std::optional<nlohmann::json> failureContext = std::nullopt) {
		std::lock_guard<std::mutex> lock(m_mutex);

		auto it = m_tasks.find(uid);
		if (it != m_tasks.end()) {
			it->second.fail(failureContext);
			m_tasks.erase(it);
		}
	}

	void cancelTask(const std::string& uid) {
		std::lock_guard<std::mutex> lock(m_mutex);

		auto it = m_tasks.find(uid);
		if (it != m_tasks.end()) {
			it->second.cancel();
			m_tasks.erase(it);
		}
	}

	void cancelAllTasks() {
		std::lock_guard<std::mutex> lock(m_mutex);
		m_tasks.clear();
	}

	bool hasTask(const std::string& uid) const {
		std::lock_guard<std::mutex> lock(m_mutex);
		return m_tasks.contains(uid);
	}

private:
	void removeTask(const std::string& uid) {
		std::lock_guard<std::mutex> lock(m_mutex);
		m_tasks.erase(uid);
	}

private:
	mutable std::mutex m_mutex;
	std::unordered_map<std::string, Task<Rep, Period>> m_tasks;
};
