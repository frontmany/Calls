#pragma once

#include "task.h"
#include "utilities/crypto.h"

#include <string>
#include <unordered_map>
#include <mutex>
#include <functional>
#include <optional>

using namespace core::utilities;

namespace core
{
	template <typename Rep, typename Period>
	class TaskManager
	{
		using TaskType = Task<Rep, Period>;

	public:
		TaskManager() = default;
		~TaskManager() = default;

		void createTask(const std::string& uid,
			const std::chrono::duration<Rep, Period>& period,
			int maxAttempts,
			std::function<void()>&& attempt,
			std::function<void(std::optional<nlohmann::json>)>&& onFinishedSuccessfully,
			std::function<void(std::optional<nlohmann::json>)>&& onFailed)
		{
			std::lock_guard<std::mutex> lock(m_mutex);

			Task task = Task(uid, period, maxAttempts, std::move(attempt), std::move(onFinishedSuccessfully), std::move(onFailed));
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
			std::optional<TaskType> task;
			{
				std::lock_guard<std::mutex> lock(m_mutex);

				if (m_tasks.contains(uid)) {
					task = std::move(m_tasks.at(uid));
					m_tasks.erase(uid);
				}
			}
			
			if (task) {
				task->complete(completionContext);
			}
		}

		void failTask(const std::string& uid, std::optional<nlohmann::json> failureContext = std::nullopt) {
			std::optional<TaskType> task;
			{
				std::lock_guard<std::mutex> lock(m_mutex);

				if (m_tasks.contains(uid)) {
					task = std::move(m_tasks.at(uid));
					m_tasks.erase(uid);
				}
			}
			
			if (task) {
				task->fail(failureContext);
			}
		}

		void cancelTask(const std::string& uid) {
			std::lock_guard<std::mutex> lock(m_mutex);

			if (m_tasks.contains(uid)) {
				auto& task = m_tasks.at(uid);
				task.cancel();
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


	private:
		mutable std::mutex m_mutex;
		std::unordered_map<std::string, TaskType> m_tasks;
	};
}