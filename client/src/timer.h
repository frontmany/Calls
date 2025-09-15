#pragma once

#include <chrono>
#include <future>
#include <atomic>
#include <functional>
#include <memory>
#include <thread>

class Timer {
public:
    Timer()
        : m_active(false),
        m_shouldStop(false)
    {
    }

    ~Timer() {
        stop();
    }

    template <class _Rep, class _Period>
    void start(std::chrono::duration<_Rep, _Period> duration, std::function<void()> onExpired) {
        if (duration < std::chrono::milliseconds(5)) {
            throw std::runtime_error("The timer cannot be set to less than 5 milliseconds");
        }

        stop();

        m_onExpired = onExpired;
        m_active = true;
        m_shouldStop = false;

        m_future = std::async(std::launch::async, [this, duration]() {
            auto start = std::chrono::steady_clock::now();
            auto end = start + duration;

            while (std::chrono::steady_clock::now() < end && !m_shouldStop) {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                if (m_shouldStop) return;
            }

            if (!m_shouldStop && m_active.load()) {
                m_onExpired();
            }
        });
    }

    void stop() {
        m_shouldStop = true;
        m_active = false;
        if (m_future.valid())
            m_future.wait();
    }

    bool is_active() const {
        return m_active.load() && !m_shouldStop;
    }

private:
    std::atomic<bool> m_active;
    std::atomic<bool> m_shouldStop;
    std::future<void> m_future;
    std::function<void()> m_onExpired;
};