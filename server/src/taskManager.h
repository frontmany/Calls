#pragma once

#include "task.h"
#include "utilities/crypto.h"

#include <string>
#include <unordered_map>
#include <mutex>
#include <functional>
#include <vector>

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
		processPendingErasesLocked();
		auto wrappedFailed = [onFailed = std::move(onFailed), this, uid](std::optional<nlohmann::json> ctx) {
			if (onFailed) onFailed(ctx);
			{ std::lock_guard<std::mutex> lock(m_mutex); m_pendingErases.push_back(uid); }
		};
		Task task = Task(uid, period, maxAttempts, std::move(attempt), std::move(onFinishedSuccessfully), std::move(wrappedFailed));
		m_tasks.emplace(uid, std::move(task));
	}

	void startTask(const std::string& uid) {
		std::lock_guard<std::mutex> lock(m_mutex);
		processPendingErasesLocked();
		if (m_tasks.contains(uid)) {
			auto& task = m_tasks.at(uid);
			task.start();
		}
	}

	void completeTask(const std::string& uid, std::optional<nlohmann::json> completionContext = std::nullopt) {
		std::lock_guard<std::mutex> lock(m_mutex);
		processPendingErasesLocked();
		if (m_tasks.contains(uid)) {
			auto& task = m_tasks.at(uid);
			task.complete(completionContext);
			m_tasks.erase(uid);
		}
	}

	void failTask(const std::string& uid, std::optional<nlohmann::json> failureContext = std::nullopt) {
		std::lock_guard<std::mutex> lock(m_mutex);
		processPendingErasesLocked();
		if (m_tasks.contains(uid)) {
			auto& task = m_tasks.at(uid);
			task.fail(failureContext);
			m_tasks.erase(uid);
		}
	}

	void cancelTask(const std::string& uid) {
		std::lock_guard<std::mutex> lock(m_mutex);
		processPendingErasesLocked();
		if (m_tasks.contains(uid)) {
			auto& task = m_tasks.at(uid);
			task.cancel();
			m_tasks.erase(uid);
		}
	}

	void cancelAllTasks() {
		std::lock_guard<std::mutex> lock(m_mutex);
		m_tasks.clear();
		m_pendingErases.clear();
	}

	bool hasTask(const std::string& uid) const {
		std::lock_guard<std::mutex> lock(m_mutex);
		return m_tasks.contains(uid);
	}

private:
	void processPendingErasesLocked() {
		for (const auto& u : m_pendingErases)
			m_tasks.erase(u);
		m_pendingErases.clear();
	}

	mutable std::mutex m_mutex;
	std::unordered_map<std::string, Task<Rep, Period>> m_tasks;
	std::vector<std::string> m_pendingErases;
};
