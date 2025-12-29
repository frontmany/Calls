#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <thread>

namespace network 
{
    class PingController {
    public:
        PingController(std::function<void()> sendPing,
            std::function<void()> onConnectionDown,
            std::function<void()> onConnectionRestored);
        ~PingController();

        void start();
        void stop();
        void handlePingSuccess();
        void setConnectionError();

    private:v 
        void pingLoop();
        void checkPing();

    private:
        std::function<void()> m_sendPing;
        std::thread m_pingThread;
        std::atomic<bool> m_running;
        std::atomic<bool> m_pingResult;
        std::atomic<bool> m_connectionError;
        std::atomic<int> m_consecutiveFailures;
        std::function<void()> m_onConnectionDown;
        std::function<void()> m_onConnectionRestored;
    };
}

