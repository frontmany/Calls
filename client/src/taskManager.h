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
			std::function<void()>&& onFinishedSuccessfully,
			std::function<void()>&& onFailed)
		{
			std::lock_guard<std::mutex> lock(m_mutex);

			Task task = Task(uid, period, maxAttempts, std::move(attempt), std::move(onFinishedSuccessfully), std::move(onFailed));
			m_tasks.emplace(uid, std::move(task));
		}

		void startTask(const std::string& uid) {
			std::lock_guard<std::mutex> lock(m_mutex);

			if (m_tasks.contains(uid)) {
				auto [uid, task] = m_tasks.at(uid);
				task.start();
			}
		}

		void completeTask(const std::string& uid) {
			std::lock_guard<std::mutex> lock(m_mutex);
			
			if (m_tasks.contains(uid)) {
				auto [uid, task] = m_tasks.at(uid);
				task.complete();
			}
		}

		void failTask(const std::string& uid) {
			std::lock_guard<std::mutex> lock(m_mutex);

			if (m_tasks.contains(uid)) {
				auto [uid, task] = m_tasks.at(uid);
				task.fail();
			}
		}

		void cancelTask(const std::string& uid) {
			std::lock_guard<std::mutex> lock(m_mutex);

			if (m_tasks.contains(uid)) {
				auto [uid, task] = m_tasks.at(uid);
				task.cancel();
			}
		}

		void cancelAllTasks() {
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