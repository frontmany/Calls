#pragma once

#include <chrono>
#include <future>
#include <atomic>
#include <functional>
#include <stdexcept>
#include <memory>
#include <thread>

namespace calls {

    class Timer {
    public:
        Timer() = default;

        Timer(const Timer&) = delete;
        Timer& operator=(const Timer&) = delete;
        Timer(Timer&&) = delete;
        Timer& operator=(Timer&&) = delete;

        ~Timer() {
            stop();
        }

        template <class Rep, class Period>
        void start(std::chrono::duration<Rep, Period> duration, std::function<void()> onExpired) {
            if (duration < std::chrono::milliseconds(1)) {
                throw std::runtime_error("Timer duration too short");
            }

            stop();

            m_active = true;

            m_future = std::async(std::launch::async,
                [duration, onExpired = std::move(onExpired), this]() {
                    auto start = std::chrono::steady_clock::now();
                    auto end = start + duration;

                    while (std::chrono::steady_clock::now() < end && m_active) {
                        auto remaining = end - std::chrono::steady_clock::now();
                        if (remaining <= std::chrono::milliseconds(0)) break;

                        auto remaining_ms = std::chrono::duration_cast<std::chrono::milliseconds>(remaining);
                        auto sleep_duration = std::chrono::milliseconds(100);

                        auto sleep_time = remaining_ms < sleep_duration ? remaining_ms : sleep_duration;

                        if (sleep_time > std::chrono::milliseconds(0)) {
                            std::this_thread::sleep_for(sleep_time);
                        }
                    }

                    if (m_active.exchange(false)) {
                        onExpired();
                    }
                });
        }

        void stop() {
            if (m_future.valid()) {
                m_active = false;
                m_future.wait();
            }
        }

        bool is_active() const {
            return m_active.load();
        }

    private:
        std::atomic<bool> m_active{ false };
        std::future<void> m_future;
    };
}