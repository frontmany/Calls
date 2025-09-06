#pragma once

#include <queue>
#include <mutex>

template <typename T>
class SafeQueue 
{
public:
	SafeQueue() = default;
	~SafeQueue() = default;

	void push(const T& data) {
		std::lock_guard<std::mutex> lock(m_mutex);
		m_queue.push(data);
	}

	void push(T&& data) {
		std::lock_guard<std::mutex> lock(m_mutex);
		m_queue.push(std::move(data));
	}

	T pop() {
		std::lock_guard<std::mutex> lock(m_mutex);
		if (m_queue.empty()) {
			throw std::runtime_error("Queue is empty");
		}
		T value = std::move(m_queue.front());
		m_queue.pop();
		return value;
	}

	const T& back() {
		std::lock_guard<std::mutex> lock(m_mutex);
		return m_queue.back();
	}

	const T& front() {
		std::lock_guard<std::mutex> lock(m_mutex);
		return m_queue.front();
	}

	size_t size() {
		std::lock_guard<std::mutex> lock(m_mutex);
		return m_queue.size();
	}

	bool empty() {
		std::lock_guard<std::mutex> lock(m_mutex);
		return m_queue.empty();
	}

private:
	std::mutex m_mutex;
	std::queue<T> m_queue;
};

