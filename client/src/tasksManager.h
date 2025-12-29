#pragma once

#include "task.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <mutex>
#include <functional>

namespace calls 
{
	class TasksManager 
	{
	public:
		TasksManager() = default;
		~TasksManager()= default;
		void createTask(const std::string& uid, int maxAttempts,
			std::function<void()>&& attempt,
			std::function<void()>&& onFailed);
		void finishTask(const std::string& uid);
		void finishAllTasks();
		bool hasTask(const std::string& uid) const;

	private:
		mutable std::mutex m_mutex;
		std::unordered_map<std::string, std::shared_ptr<Task>> m_tasks;
	};
}