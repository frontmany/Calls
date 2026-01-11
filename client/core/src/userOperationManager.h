#pragma once

#include <vector>
#include <mutex>
#include <string>
#include "userOperation.h"

namespace core
{
    class UserOperationManager
    {
    public:
        UserOperationManager() = default;
        ~UserOperationManager() = default;

        bool isOperation(UserOperationType type, const std::string& nickname = "") const;
        bool isOperationType(UserOperationType type) const;
        void addOperation(UserOperationType type, const std::string& nickname = "");
        void removeOperation(UserOperationType type, const std::string& nickname = "");
        void clearAllOperations();

    private:
        std::vector<UserOperation> m_pendingOperations;
        mutable std::mutex m_mutex;
    };
}