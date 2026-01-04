#include "pingController.h"
#include "utilities/logger.h"

#include <algorithm>

namespace server
{
    namespace network 
    {
    PingController::PingController(std::function<void(const asio::ip::udp::endpoint&)> sendPing,
        std::function<void(const asio::ip::udp::endpoint&)> onConnectionDown,
        std::function<void(const asio::ip::udp::endpoint&)> onConnectionRestored)
        : m_sendPing(std::move(sendPing)),
          m_onConnectionDown(std::move(onConnectionDown)),
          m_onConnectionRestored(std::move(onConnectionRestored)),
          m_running(false)
    {
    }

    PingController::~PingController() {
        stop();
    }

    void PingController::start() {
        if (m_running.exchange(true)) {
            return;
        }

        m_pingThread = std::thread([this]() {
            pingLoop();
        });
    }

    void PingController::stop() {
        if (!m_running.exchange(false)) {
            return;
        }

        if (m_pingThread.joinable()) {
            m_pingThread.join();
        }
    }

    std::string PingController::makeEndpointKey(const asio::ip::udp::endpoint& endpoint)
    {
        return endpoint.address().to_string() + ":" + std::to_string(endpoint.port());
    }

    void PingController::addEndpoint(const asio::ip::udp::endpoint& endpoint) {
        std::lock_guard<std::mutex> lock(m_endpointsMutex);
        std::string key = makeEndpointKey(endpoint);
        m_endpointsToPing.insert(key);
        
        auto [it, inserted] = m_pingStates.try_emplace(key);
        if (inserted) {
            it->second.pingResult = false;
            it->second.consecutiveFailures = 0;
            it->second.connectionError = false;
        }
    }

    void PingController::removeEndpoint(const asio::ip::udp::endpoint& endpoint) {
        std::lock_guard<std::mutex> lock(m_endpointsMutex);
        std::string key = makeEndpointKey(endpoint);
        m_endpointsToPing.erase(key);
        m_pingStates.erase(key);
    }

    void PingController::handlePingSuccess(const asio::ip::udp::endpoint& endpoint) {
        std::lock_guard<std::mutex> lock(m_endpointsMutex);
        std::string key = makeEndpointKey(endpoint);
        auto it = m_pingStates.find(key);
        if (it != m_pingStates.end()) {
            it->second.pingResult = true;
        }
    }

    void PingController::pingLoop() {
        using namespace std::chrono_literals;

        std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
        std::chrono::milliseconds pingGap = 500ms;
        std::chrono::seconds checkPingGap = 1s;

        std::chrono::steady_clock::time_point lastPing = begin;
        std::chrono::steady_clock::time_point lastCheck = begin;

        while (m_running.load()) {
            std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();

            auto timeSinceLastCheck = now - lastCheck;
            auto timeSinceLastPing = now - lastPing;

            if (timeSinceLastCheck >= checkPingGap) {
                checkPing();
                lastCheck = now;
                lastPing = now;
            }
            else if (timeSinceLastPing >= pingGap) {
                {
                    std::lock_guard<std::mutex> lock(m_endpointsMutex);
                    for (const auto& endpointKey : m_endpointsToPing) {
                        auto it = m_pingStates.find(endpointKey);
                        if (it != m_pingStates.end()) {
                            it->second.pingResult = false;
                        }
                    }
                }

                std::vector<asio::ip::udp::endpoint> endpointsToPing;
                {
                    std::lock_guard<std::mutex> lock(m_endpointsMutex);
                    for (const auto& endpointKey : m_endpointsToPing) {
                        size_t colonPos = endpointKey.find(':');
                        if (colonPos != std::string::npos) {
                            std::string addressStr = endpointKey.substr(0, colonPos);
                            std::string portStr = endpointKey.substr(colonPos + 1);
                            try {
                                asio::ip::address address = asio::ip::address::from_string(addressStr);
                                unsigned short port = static_cast<unsigned short>(std::stoi(portStr));
                                endpointsToPing.push_back(asio::ip::udp::endpoint(address, port));
                            }
                            catch (const std::exception& e) {
                                LOG_WARN("Failed to parse endpoint {}: {}", endpointKey, e.what());
                            }
                        }
                    }
                }

                if (m_sendPing) {
                    for (const auto& endpoint : endpointsToPing) {
                        m_sendPing(endpoint);
                    }
                }
                lastPing = now;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    void PingController::checkPing() {
        const int MAX_CONSECUTIVE_FAILURES = 2;

        std::unique_lock<std::mutex> lock(m_endpointsMutex);

        std::vector<std::pair<std::string, asio::ip::udp::endpoint>> endpointsToNotifyDown;
        std::vector<std::pair<std::string, asio::ip::udp::endpoint>> endpointsToNotifyRestored;

        for (auto& [endpointKey, pingState] : m_pingStates) {
            if (m_endpointsToPing.find(endpointKey) == m_endpointsToPing.end()) {
                continue;
            }

            size_t colonPos = endpointKey.find(':');
            if (colonPos == std::string::npos) {
                continue;
            }

            std::string addressStr = endpointKey.substr(0, colonPos);
            std::string portStr = endpointKey.substr(colonPos + 1);
            asio::ip::udp::endpoint endpoint;
            try {
                asio::ip::address address = asio::ip::address::from_string(addressStr);
                unsigned short port = static_cast<unsigned short>(std::stoi(portStr));
                endpoint = asio::ip::udp::endpoint(address, port);
            }
            catch (const std::exception& e) {
                LOG_WARN("Failed to parse endpoint {}: {}", endpointKey, e.what());
                continue;
            }

            if (pingState.pingResult.load()) {
                pingState.consecutiveFailures = 0;

                if (pingState.connectionError.load()) {
                    endpointsToNotifyRestored.push_back({ endpointKey, endpoint });
                    pingState.connectionError = false;
                }

                pingState.pingResult = false;
                LOG_DEBUG("Ping success for endpoint {}", endpointKey);
            }
            else {
                int failures = pingState.consecutiveFailures.fetch_add(1) + 1;
                LOG_WARN("Ping check failed for endpoint {} (consecutive failures: {})", endpointKey, failures);

                if (failures >= MAX_CONSECUTIVE_FAILURES) {
                    if (!pingState.connectionError.load()) {
                        endpointsToNotifyDown.push_back({ endpointKey, endpoint });
                    }
                    pingState.connectionError = true;
                }
            }
        }

        lock.unlock();

        for (const auto& [endpointKey, endpoint] : endpointsToNotifyDown) {
            if (m_onConnectionDown) {
                m_onConnectionDown(endpoint);
            }
        }

        for (const auto& [endpointKey, endpoint] : endpointsToNotifyRestored) {
            if (m_onConnectionRestored) {
                m_onConnectionRestored(endpoint);
            }
        }
    }
    }
}

