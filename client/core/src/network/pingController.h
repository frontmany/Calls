#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <thread>

namespace core
{
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
        /** Reset failure state after successful reconnect. Prevents immediate "connection down by ping"
            when RECONNECT_RESULT arrives during a period where pings may have been delayed. */
        void resetAfterReconnect();

    private:
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
}

