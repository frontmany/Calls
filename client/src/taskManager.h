#pragma once

#include "task.h"
#include "utilities/crypto.h"

#include <string>
#include <unordered_map>
#include <mutex>
#include <functional>


using namespace utilities;

namespace calls
{
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
			std::lock_guard<std::mutex> lock(m_mutex);
			
			if (m_tasks.contains(uid)) {
				auto& task = m_tasks.at(uid);
				task.complete(completionContext);
			}
		}

		void failTask(const std::string& uid, std::optional<nlohmann::json> failureContext = std::nullopt) {
			std::lock_guard<std::mutex> lock(m_mutex);

			if (m_tasks.contains(uid)) {
				auto& task = m_tasks.at(uid);
				task.fail(failureContext);
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
		std::unordered_map<std::string, calls::Task<Rep, Period>> m_tasks;
	};
}