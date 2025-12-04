#include "pingManager.h"
#include "networkController.h"
#include "packetTypes.h"
#include "logger.h"

namespace calls {
    PingManager::PingManager(std::shared_ptr<NetworkController> networkController,
        std::function<void()>&& onPingFail, std::function<void()>&& onPingEstablishedAgain)
        : m_networkController(networkController), m_onPingEstablishedAgain(onPingEstablishedAgain),
        m_onPingFail(onPingFail)
    {
    }

    PingManager::~PingManager() {
        m_running = false;
        if (m_pingThread.joinable()) {
            m_pingThread.join();
        }
    }

    void PingManager::start() {
        m_pingThread = std::thread(&PingManager::ping, this);
        m_running = true;
    }

    void PingManager::stop() {
        m_running = false;
        if (m_pingThread.joinable()) {
            m_pingThread.join();
        }
    }

    void PingManager::setPingSuccess() {
        m_pingResult = true;
    }

    void PingManager::ping() {
        using namespace std::chrono_literals;

        std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
        std::chrono::seconds pingGap = 1s;
        std::chrono::seconds checkPingGap = 5s;

        std::chrono::steady_clock::time_point lastPing = begin;
        std::chrono::steady_clock::time_point lastCheck = begin;

        while (m_running) {
            std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();

            auto timeSinceLastCheck = now - lastCheck;
            auto timeSinceLastPing = now - lastPing;

            if (timeSinceLastCheck >= checkPingGap) {
                checkPing();
                lastCheck = now;
                lastPing = now;
            }
            else if (timeSinceLastPing >= pingGap) {
                m_networkController->send(PacketType::PING);
                lastPing = now;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }

    void PingManager::checkPing() {
        const int MAX_CONSECUTIVE_FAILURES = 4;

        if (m_pingResult) {
            m_consecutiveFailures = 0;
            
            if (m_error) {
                m_onPingEstablishedAgain();
                m_error = false;
            }

            m_pingResult = false;
            LOG_INFO("ping success");
        }
        else {
            m_consecutiveFailures++;
            LOG_WARN("ping check failed (consecutive failures: {})", m_consecutiveFailures.load());
            
            if (m_consecutiveFailures >= MAX_CONSECUTIVE_FAILURES) {
                if (!m_error) {
                    LOG_ERROR("Multiple consecutive ping failures, triggering network error");
                    m_onPingFail();
                }
                m_error = true;
            }
        }
    }
}