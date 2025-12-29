#include "tasksManager.h"

namespace calls 
{
	void TasksManager::createTask(const std::string& uid, int maxAttempts,
		std::function<void()>&& attempt,
		std::function<void()>&& onFailed)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		auto task = std::make_shared<Task>(uid, maxAttempts, std::move(attempt), std::move(onFailed));
		m_tasks.emplace(uid, task);
		task->start();
	}

	void TasksManager::finishTask(const std::string& uid) {
		std::lock_guard<std::mutex> lock(m_mutex);
		auto it = m_tasks.find(uid);
		if (it != m_tasks.end())
		{
			it->second->setFinished();
			m_tasks.erase(it);
		}
	} 

	void TasksManager::finishAllTasks() {
		std::lock_guard<std::mutex> lock(m_mutex);
		for (auto& [uid, task] : m_tasks)
		{
			task->setFinished();
		}
		m_tasks.clear();
	}

	bool TasksManager::hasTask(const std::string& uid) const {
		std::lock_guard<std::mutex> lock(m_mutex);
		return m_tasks.find(uid) != m_tasks.end();
	}
}

