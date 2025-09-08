#if defined(_WIN32)
#define _WIN32_WINNT 0x0A00 
#endif

#include "callsClient.h"

#include <iostream>
#include <string>
#include <sstream>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>

class ConsoleApp {
private:
    std::unique_ptr<CallsClient> m_client;
    std::atomic<bool> m_running{true};
    std::mutex m_consoleMutex;
    std::condition_variable m_cv;
    std::string m_currentNickname;
    std::string m_currentFriendNickname;
    bool m_isAuthorized = false;
    bool m_isInCall = false;

public:
    ConsoleApp() {
        // Initialize callbacks
        auto authCallback = [this](AuthorizationResult result) {
            std::lock_guard<std::mutex> lock(m_consoleMutex);
            switch (result) {
                case AuthorizationResult::SUCCESS:
                    std::cout << "✓ Authorization successful!" << std::endl;
                    m_isAuthorized = true;
                    break;
                case AuthorizationResult::FAIL:
                    std::cout << "✗ Authorization failed!" << std::endl;
                    m_isAuthorized = false;
                    break;
                case AuthorizationResult::TIMEOUT:
                    std::cout << "⏰ Authorization timeout!" << std::endl;
                    m_isAuthorized = false;
                    break;
            }
            m_cv.notify_one();
        };

        auto createCallCallback = [this](CreateCallResult result) {
            std::lock_guard<std::mutex> lock(m_consoleMutex);
            switch (result) {
                case CreateCallResult::CALL_ACCEPTED:
                    std::cout << "✓ Call accepted by " << m_currentFriendNickname << "!" << std::endl;
                    m_isInCall = true;
                    break;
                case CreateCallResult::CALL_DECLINED:
                    std::cout << "✗ Call declined by " << m_currentFriendNickname << std::endl;
                    m_isInCall = false;
                    break;
                case CreateCallResult::WRONG_FRIEND_NICKNAME_RESULT:
                    std::cout << "✗ Friend not found: " << m_currentFriendNickname << std::endl;
                    m_isInCall = false;
                    break;
                case CreateCallResult::TIMEOUT:
                    std::cout << "⏰ Call timeout!" << std::endl;
                    m_isInCall = false;
                    break;
                case CreateCallResult::UNAUTHORIZED:
                    std::cout << "✗ Not authorized!" << std::endl;
                    m_isInCall = false;
                    break;
            }
            m_cv.notify_one();
        };

        auto incomingCallCallback = [this](const IncomingCallData& data) {
            std::lock_guard<std::mutex> lock(m_consoleMutex);
            std::cout << "\n📞 Incoming call from: " << data.friendNickname << std::endl;
            std::cout << "Type 'accept " << data.friendNickname << "' to accept or 'decline " << data.friendNickname << "' to decline" << std::endl;
        };

        auto hangUpCallback = [this]() {
            std::lock_guard<std::mutex> lock(m_consoleMutex);
            std::cout << "📞 Call ended" << std::endl;
            m_isInCall = false;
        };

        auto networkErrorCallback = [this]() {
            std::lock_guard<std::mutex> lock(m_consoleMutex);
            std::cout << "⚠️ Network error occurred!" << std::endl;
        };

        m_client = std::make_unique<CallsClient>(
            authCallback,
            createCallCallback,
            incomingCallCallback,
            hangUpCallback,
            networkErrorCallback
        );
    }

    void run() {
        std::cout << "=== Calls Client Console Application ===" << std::endl;
        std::cout << "Available commands:" << std::endl;
        std::cout << "  auth <nickname>     - Authorize with nickname" << std::endl;
        std::cout << "  call <friend>       - Call a friend" << std::endl;
        std::cout << "  accept <friend>     - Accept incoming call" << std::endl;
        std::cout << "  decline <friend>    - Decline incoming call" << std::endl;
        std::cout << "  hangup              - End current call" << std::endl;
        std::cout << "  status              - Show current status" << std::endl;
        std::cout << "  help                - Show this help" << std::endl;
        std::cout << "  quit                - Exit application" << std::endl;
        std::cout << std::endl;

        std::string input;
        while (m_running) {
            std::cout << "> ";
            std::getline(std::cin, input);
            
            if (input.empty()) continue;
            
            processCommand(input);
        }
    }

private:
    void processCommand(const std::string& input) {
        std::istringstream iss(input);
        std::string command;
        iss >> command;

        if (command == "quit" || command == "exit") {
            std::cout << "Goodbye!" << std::endl;
            m_running = false;
            return;
        }
        else if (command == "help") {
            showHelp();
        }
        else if (command == "status") {
            showStatus();
        }
        else if (command == "auth") {
            std::string nickname;
            if (iss >> nickname) {
                authorize(nickname);
            } else {
                std::cout << "Usage: auth <nickname>" << std::endl;
            }
        }
        else if (command == "call") {
            std::string friendNickname;
            if (iss >> friendNickname) {
                callFriend(friendNickname);
            } else {
                std::cout << "Usage: call <friend_nickname>" << std::endl;
            }
        }
        else if (command == "accept") {
            std::string friendNickname;
            if (iss >> friendNickname) {
                acceptCall(friendNickname);
            } else {
                std::cout << "Usage: accept <friend_nickname>" << std::endl;
            }
        }
        else if (command == "decline") {
            std::string friendNickname;
            if (iss >> friendNickname) {
                declineCall(friendNickname);
            } else {
                std::cout << "Usage: decline <friend_nickname>" << std::endl;
            }
        }
        else if (command == "hangup") {
            hangUp();
        }
        else {
            std::cout << "Unknown command: " << command << std::endl;
            std::cout << "Type 'help' for available commands" << std::endl;
        }
    }

    void showHelp() {
        std::cout << "\nAvailable commands:" << std::endl;
        std::cout << "  auth <nickname>     - Authorize with nickname" << std::endl;
        std::cout << "  call <friend>       - Call a friend" << std::endl;
        std::cout << "  accept <friend>     - Accept incoming call" << std::endl;
        std::cout << "  decline <friend>    - Decline incoming call" << std::endl;
        std::cout << "  hangup              - End current call" << std::endl;
        std::cout << "  status              - Show current status" << std::endl;
        std::cout << "  help                - Show this help" << std::endl;
        std::cout << "  quit                - Exit application" << std::endl;
        std::cout << std::endl;
    }

    void showStatus() {
        std::cout << "\n=== Status ===" << std::endl;
        std::cout << "Authorized: " << (m_isAuthorized ? "Yes" : "No") << std::endl;
        if (!m_currentNickname.empty()) {
            std::cout << "Nickname: " << m_currentNickname << std::endl;
        }
        std::cout << "In call: " << (m_isInCall ? "Yes" : "No") << std::endl;
        if (!m_currentFriendNickname.empty() && m_isInCall) {
            std::cout << "Calling: " << m_currentFriendNickname << std::endl;
        }
        std::cout << std::endl;
    }

    void authorize(const std::string& nickname) {
        if (m_isAuthorized) {
            std::cout << "Already authorized as: " << m_currentNickname << std::endl;
            return;
        }

        std::cout << "Authorizing as: " << nickname << "..." << std::endl;
        m_currentNickname = nickname;
        m_client->authorize(nickname);

        // Wait for authorization result
        std::unique_lock<std::mutex> lock(m_consoleMutex);
        m_cv.wait(lock, [this] { return !m_running || m_isAuthorized || m_currentNickname.empty(); });
    }

    void callFriend(const std::string& friendNickname) {
        if (!m_isAuthorized) {
            std::cout << "Please authorize first using 'auth <nickname>'" << std::endl;
            return;
        }

        if (m_isInCall) {
            std::cout << "Already in a call. Use 'hangup' to end current call first." << std::endl;
            return;
        }

        std::cout << "Calling " << friendNickname << "..." << std::endl;
        m_currentFriendNickname = friendNickname;
        m_client->createCall(friendNickname);

        // Wait for call result
        std::unique_lock<std::mutex> lock(m_consoleMutex);
        m_cv.wait(lock, [this] { return !m_running || m_isInCall || m_currentFriendNickname.empty(); });
    }

    void acceptCall(const std::string& friendNickname) {
        if (!m_isAuthorized) {
            std::cout << "Please authorize first using 'auth <nickname>'" << std::endl;
            return;
        }

        if (m_isInCall) {
            std::cout << "Already in a call. Use 'hangup' to end current call first." << std::endl;
            return;
        }

        std::cout << "Accepting call from " << friendNickname << "..." << std::endl;
        m_currentFriendNickname = friendNickname;
        m_client->acceptIncomingCall(friendNickname);
        m_isInCall = true;
    }

    void declineCall(const std::string& friendNickname) {
        if (!m_isAuthorized) {
            std::cout << "Please authorize first using 'auth <nickname>'" << std::endl;
            return;
        }

        std::cout << "Declining call from " << friendNickname << "..." << std::endl;
        m_client->declineIncomingCall(friendNickname);
    }

    void hangUp() {
        if (!m_isInCall) {
            std::cout << "Not in a call" << std::endl;
            return;
        }

        std::cout << "Ending call..." << std::endl;
        m_client->endCall();
        m_isInCall = false;
        m_currentFriendNickname.clear();
    }
};

int main() {
    try {
        ConsoleApp app;
        app.run();
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}