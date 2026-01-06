#include "userOperationManager.h"
#include <algorithm>

namespace core
{
    bool UserOperationManager::isOperation(UserOperationType type, const std::string& nickname) const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        UserOperation operation(type, nickname);
        return std::find(m_pendingOperations.begin(), m_pendingOperations.end(), operation) != m_pendingOperations.end();
    }

    bool UserOperationManager::isOperationType(UserOperationType type) const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return std::any_of(m_pendingOperations.begin(), m_pendingOperations.end(),
            [type](const UserOperation& op) {
                return op.type == type;
            });
    }

    void UserOperationManager::addOperation(UserOperationType type, const std::string& nickname)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        UserOperation operation(type, nickname);
        m_pendingOperations.push_back(operation);
    }

    void UserOperationManager::removeOperation(UserOperationType type, const std::string& nickname)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        UserOperation operation(type, nickname);
        m_pendingOperations.erase(
            std::remove(m_pendingOperations.begin(), m_pendingOperations.end(), operation),
            m_pendingOperations.end()
        );
    }

    void UserOperationManager::clearAllOperations()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_pendingOperations.clear();
    }
}
