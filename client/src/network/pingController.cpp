#include "pingController.h"
#include "logger.h"

namespace network 
{
    PingController::PingController(std::function<void()> sendPing,
        std::function<void()> onConnectionDown,
        std::function<void()> onConnectionRestored)
        : m_sendPing(std::move(sendPing)),
          m_onConnectionDown(std::move(onConnectionDown)),
          m_onConnectionRestored(std::move(onConnectionRestored)),
          m_running(false),
          m_pingResult(false),
          m_connectionError(false),
          m_consecutiveFailures(0)
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

    void PingController::handlePingSuccess() {
        m_pingResult = true;
    }

    void PingController::setConnectionError() {
        if (!m_connectionError.load()) {
            m_connectionError = true;
            LOG_INFO("Connection error set in PingController");
        }
    }

    void PingController::pingLoop() {
        using namespace std::chrono_literals;

        std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
        std::chrono::seconds pingGap = 1s;
        std::chrono::seconds checkPingGap = 5s;

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
                if (m_sendPing) {
                    m_sendPing();
                }
                lastPing = now;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }

    void PingController::checkPing() {
        const int MAX_CONSECUTIVE_FAILURES = 4;

        if (m_pingResult.load()) {
            m_consecutiveFailures = 0;

            if (m_connectionError.load()) {
                if (m_onConnectionRestored) {
                    m_onConnectionRestored();
                }
                m_connectionError = false;
            }

            m_pingResult = false;
            LOG_INFO("ping success");
        }
        else {
            int failures = m_consecutiveFailures.fetch_add(1) + 1;
            LOG_WARN("ping check failed (consecutive failures: {})", failures);

            if (failures >= MAX_CONSECUTIVE_FAILURES) {
                if (!m_connectionError.load()) {
                    LOG_ERROR("Multiple consecutive ping failures, triggering connection down");
                    if (m_onConnectionDown) {
                        m_onConnectionDown();
                    }
                }
                m_connectionError = true;
            }
        }
    }
}

