#pragma once

#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

#include "json.hpp"

namespace core
{
    class PendingRequests {
    public:
        using OnComplete = std::function<void(std::optional<nlohmann::json>)>;
        using OnFail = std::function<void(std::optional<nlohmann::json>)>;

        void add(const std::string& uid, OnComplete onComplete, OnFail onFail);
        bool complete(const std::string& uid, std::optional<nlohmann::json> context);
        bool fail(const std::string& uid, std::optional<nlohmann::json> context = std::nullopt);
        void failAll(std::optional<nlohmann::json> context = std::nullopt);
        bool has(const std::string& uid) const;

    private:
        mutable std::mutex m_mutex;
        std::unordered_map<std::string, std::pair<OnComplete, OnFail>> m_pending;
    };
}
