#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>

#include "asio.hpp"

namespace network 
{
    class PingController {
    private:
        struct PingState {
            std::atomic<bool> pingResult;
            std::atomic<int> consecutiveFailures;
            std::atomic<bool> connectionError;
        };

    public:
        PingController(std::function<void(const asio::ip::udp::endpoint&)> sendPing,
            std::function<void(const asio::ip::udp::endpoint&)> onConnectionDown,
            std::function<void(const asio::ip::udp::endpoint&)> onConnectionRestored);
        ~PingController();

        void start();
        void stop();
        void handlePingSuccess(const asio::ip::udp::endpoint& endpoint);
        void addEndpoint(const asio::ip::udp::endpoint& endpoint);
        void removeEndpoint(const asio::ip::udp::endpoint& endpoint);

    private:
        void pingLoop();
        void checkPing();
        std::string makeEndpointKey(const asio::ip::udp::endpoint& endpoint);

    private:
        std::thread m_pingThread;
        std::atomic<bool> m_running;
        std::mutex m_endpointsMutex;
        std::unordered_map<std::string, PingState> m_pingStates;
        std::unordered_set<std::string> m_endpointsToPing;
        std::function<void(const asio::ip::udp::endpoint&)> m_sendPing;
        std::function<void(const asio::ip::udp::endpoint&)> m_onConnectionDown;
        std::function<void(const asio::ip::udp::endpoint&)> m_onConnectionRestored;
    };
}

