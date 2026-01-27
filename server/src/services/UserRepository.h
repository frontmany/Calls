#pragma once

#include <unordered_map>
#include <mutex>
#include <memory>
#include <string>
#include "IUserRepository.h"
#include "user.h"
#include "asio.hpp"

namespace server
{
    namespace services
    {
        // Реализация репозитория пользователей
        class UserRepository : public IUserRepository {
        public:
            UserRepository() = default;
            ~UserRepository() = default;

            UserPtr findUserByNickname(const std::string& nicknameHash) override {
                std::lock_guard<std::mutex> lock(m_mutex);
                auto it = m_nicknameHashToUser.find(nicknameHash);
                if (it != m_nicknameHashToUser.end()) {
                    return it->second;
                }
                return nullptr;
            }

            UserPtr findUserByEndpoint(const asio::ip::udp::endpoint& endpoint) override {
                std::lock_guard<std::mutex> lock(m_mutex);
                auto it = m_endpointToUser.find(endpoint);
                if (it != m_endpointToUser.end()) {
                    return it->second;
                }
                return nullptr;
            }

            void addUser(UserPtr user) override {
                if (!user) return;

                std::lock_guard<std::mutex> lock(m_mutex);
                m_nicknameHashToUser[user->getNicknameHash()] = user;
                m_endpointToUser[user->getEndpoint()] = user;
            }

            void removeUser(const std::string& nicknameHash) override {
                std::lock_guard<std::mutex> lock(m_mutex);
                
                auto nicknameIt = m_nicknameHashToUser.find(nicknameHash);
                if (nicknameIt != m_nicknameHashToUser.end()) {
                    auto endpoint = nicknameIt->second->getEndpoint();
                    m_endpointToUser.erase(endpoint);
                    m_nicknameHashToUser.erase(nicknameIt);
                }
            }

            bool containsUser(const std::string& nicknameHash) const override {
                std::lock_guard<std::mutex> lock(m_mutex);
                return m_nicknameHashToUser.find(nicknameHash) != m_nicknameHashToUser.end();
            }

            bool containsUser(const asio::ip::udp::endpoint& endpoint) const override {
                std::lock_guard<std::mutex> lock(m_mutex);
                return m_endpointToUser.find(endpoint) != m_endpointToUser.end();
            }

            void updateUserEndpoint(const std::string& nicknameHash, 
                                   const asio::ip::udp::endpoint& newEndpoint) override {
                std::lock_guard<std::mutex> lock(m_mutex);
                
                auto it = m_nicknameHashToUser.find(nicknameHash);
                if (it != m_nicknameHashToUser.end()) {
                    // Удалить старый endpoint
                    auto oldEndpoint = it->second->getEndpoint();
                    m_endpointToUser.erase(oldEndpoint);
                    
                    // Обновить endpoint в пользователе
                    it->second->setEndpoint(newEndpoint);
                    
                    // Добавить новый endpoint
                    m_endpointToUser[newEndpoint] = it->second;
                }
            }

            // Получить прямой доступ к мапам (для обратной совместимости во время рефакторинга)
            std::unordered_map<asio::ip::udp::endpoint, UserPtr>& getEndpointToUser() {
                return m_endpointToUser;
            }

            std::unordered_map<std::string, UserPtr>& getNicknameHashToUser() {
                return m_nicknameHashToUser;
            }

            std::mutex& getMutex() {
                return m_mutex;
            }

        private:
            mutable std::mutex m_mutex;
            std::unordered_map<asio::ip::udp::endpoint, UserPtr> m_endpointToUser;
            std::unordered_map<std::string, UserPtr> m_nicknameHashToUser;
        };
    }
}
