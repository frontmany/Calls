#include "pendingRequests.h"

namespace core
{
    void PendingRequests::add(const std::string& uid, OnComplete onComplete, OnFail onFail) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_pending.emplace(uid, std::make_pair(std::move(onComplete), std::move(onFail)));
    }

    bool PendingRequests::complete(const std::string& uid, std::optional<nlohmann::json> context) {
        std::pair<OnComplete, OnFail> handlers;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto it = m_pending.find(uid);
            if (it == m_pending.end())
                return false;
            handlers = std::move(it->second);
            m_pending.erase(it);
        }
        if (handlers.first)
            handlers.first(std::move(context));
        return true;
    }

    bool PendingRequests::fail(const std::string& uid, std::optional<nlohmann::json> context) {
        std::pair<OnComplete, OnFail> handlers;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto it = m_pending.find(uid);
            if (it == m_pending.end())
                return false;
            handlers = std::move(it->second);
            m_pending.erase(it);
        }
        if (handlers.second)
            handlers.second(std::move(context));
        return true;
    }

    void PendingRequests::failAll(std::optional<nlohmann::json> context) {
        std::unordered_map<std::string, std::pair<OnComplete, OnFail>> copy;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            copy = std::move(m_pending);
            m_pending.clear();
        }
        for (auto& [_, p] : copy) {
            if (p.second)
                p.second(context);
        }
    }

    bool PendingRequests::has(const std::string& uid) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_pending.count(uid) != 0;
    }
}
