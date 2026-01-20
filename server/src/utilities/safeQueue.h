#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>
#include <chrono>
#include <memory>
#include <functional>

namespace server
{
    namespace utilities
    {
        template<typename T>
        class SafeQueue {
    public:
        SafeQueue() = default;
        SafeQueue(const SafeQueue&) = delete;
        SafeQueue& operator=(const SafeQueue&) = delete;

        void push(const T& item) {
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_queue.push(item);
            }
            m_cond.notify_one();
        }

        void push(T&& item) {
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_queue.push(std::move(item));
            }
            m_cond.notify_one();
        }

        template<typename... Args>
        void emplace(Args&&... args) {
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_queue.emplace(std::forward<Args>(args)...);
            }
            m_cond.notify_one();
        }

        std::optional<T> try_pop() {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_queue.empty()) {
                return std::nullopt;
            }
            T item = std::move(m_queue.front());
            m_queue.pop();
            return item;
        }

        template<typename Rep, typename Period>
        std::optional<T> pop_for(const std::chrono::duration<Rep, Period>& timeout) {
            std::unique_lock<std::mutex> lock(m_mutex);
            if (!m_cond.wait_for(lock, timeout, [this]() { return !m_queue.empty(); })) {
                return std::nullopt;
            }
            T item = std::move(m_queue.front());
            m_queue.pop();
            return item;
        }

        const T* front_ptr() const {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_queue.empty()) {
                return nullptr;
            }
            return &m_queue.front();
        }

        const T* back_ptr() const {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_queue.empty()) {
                return nullptr;
            }
            return &m_queue.back();
        }

        T* front_ptr() {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_queue.empty()) {
                return nullptr;
            }
            return &m_queue.front();
        }

        T* back_ptr() {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_queue.empty()) {
                return nullptr;
            }
            return &m_queue.back();
        }

        const T& unsafe_front() const {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_queue.front();
        }

        const T& unsafe_back() const {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_queue.back();
        }


        std::optional<T> front() const {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_queue.empty()) {
                return std::nullopt;
            }
            return m_queue.front();
        }

        std::optional<T> back() const {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_queue.empty()) {
                return std::nullopt;
            }
            return m_queue.back();
        }

        size_t size() const {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_queue.size();
        }

        bool empty() const {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_queue.empty();
        }

        void clear() {
            std::lock_guard<std::mutex> lock(m_mutex);
            while (!m_queue.empty()) {
                m_queue.pop();
            }
        }

        void swap(std::queue<T>& other) {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_queue.swap(other);
        }

    private:
        mutable std::mutex m_mutex;
        std::queue<T> m_queue;
        std::condition_variable m_cond;
    };
    }
}