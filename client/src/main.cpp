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
    std::mutex m_consoleMutex;
    std::condition_variable m_cv;
    std::string m_currentFriendNickname;
    bool m_isMuted = false;

public:
    ConsoleApp() {
        auto authCallback = [this](AuthorizationResult result) {
            std::lock_guard<std::mutex> lock(m_consoleMutex);
            switch (result) {
            case AuthorizationResult::SUCCESS:
                std::cout << "Authorization successfull!" << std::endl;
                break;
            case AuthorizationResult::FAIL:
                std::cout << "Authorization failed!" << std::endl;
                break;
            case AuthorizationResult::TIMEOUT:
                std::cout << "Authorization timeout!" << std::endl;
                break;
            }
            m_cv.notify_one();
            };

        auto createCallCallback = [this](CreateCallResult result) {
            std::lock_guard<std::mutex> lock(m_consoleMutex);
            switch (result) {
            case CreateCallResult::CALL_ACCEPTED:
                std::cout << "Call accepted by " << m_currentFriendNickname << "!" << std::endl;
                break;
            case CreateCallResult::CALL_DECLINED:
                std::cout << "Call declined by " << m_currentFriendNickname << std::endl;
                m_currentFriendNickname.clear();
                break;
            case CreateCallResult::WRONG_FRIEND_NICKNAME:
                std::cout << "Friend not found: " << m_currentFriendNickname << std::endl;
                m_currentFriendNickname.clear();
                break;
            case CreateCallResult::TIMEOUT:
                std::cout << "Call timeout!" << std::endl;
                m_currentFriendNickname.clear();
                break;
            case CreateCallResult::UNAUTHORIZED:
                std::cout << "Not authorized!" << std::endl;
                m_currentFriendNickname.clear();
                break;
            }
            m_cv.notify_one();
            };

        auto incomingCallCallback = [this](const IncomingCallData& data) {
            std::lock_guard<std::mutex> lock(m_consoleMutex);
            std::cout << "\nIncoming call from: " << data.friendNickname << std::endl;
            std::cout << "Type 'accept " << data.friendNickname << "' to accept or 'decline " << data.friendNickname << "' to decline" << std::endl;
            std::cout << "> ";
            };

        auto incomingCallingExpiredCallback = [this](const std::string& friendNickname) {
            std::lock_guard<std::mutex> lock(m_consoleMutex);
            std::cout << "Incoming call from " << friendNickname << " expired" << std::endl;
            };

        auto hangUpCallback = [this]() {
            std::lock_guard<std::mutex> lock(m_consoleMutex);
            std::cout << "Call ended" << std::endl;
            std::cout << "> ";
            };

        auto networkErrorCallback = [this]() {
            std::lock_guard<std::mutex> lock(m_consoleMutex);
            std::cout << "Network error occurred!" << std::endl;
            m_cv.notify_one();
        };

        m_client = std::make_unique<CallsClient>("192.168.1.45",
            authCallback,
            createCallCallback,
            incomingCallCallback,
            incomingCallingExpiredCallback,
            hangUpCallback,
            networkErrorCallback
        );

        m_client->run();
    }

    void run() {
        std::cout << "auth      <nickname>  - Authorize with nickname" << std::endl;
        std::cout << "call      <friend>    - Call a friend" << std::endl;
        std::cout << "accept    <friend>    - Accept incoming call" << std::endl;
        std::cout << "decline   <friend>    - Decline incoming call" << std::endl;
        std::cout << "hangup                - End current call" << std::endl;
        std::cout << "mute      <on/off>    - Mute/unmute microphone" << std::endl;
        std::cout << "quit                  - Exit application" << std::endl;
        std::cout << std::endl;

        std::string input;
        while (m_client->isRunning()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(600));
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
            m_client->stop();
            std::cout << "Goodbye!" << std::endl;
            return;
        }
        else if (command == "auth") {
            std::string nickname;
            if (iss >> nickname) {
                authorize(nickname);
            }
            else {
                std::cout << "\nIncorrect command usage: authorize <nickname>" << std::endl;
                std::cout << "Parameters:  <nickname> - your unique identifier (3-20 characters, letters and numbers only)" << std::endl;
                std::cout << "Example:  authorize Alice123\n" << std::endl;
            }
        }
        else if (command == "call") {
            std::string friendNickname;
            if (iss >> friendNickname) {
                callFriend(friendNickname);
            }
            else {
                std::cout << "\nIncorrect command usage: call <friend_nickname>" << std::endl;
                std::cout << "Parameters:  <friend_nickname> - nickname of the user you want to call" << std::endl;
                std::cout << "Example:  call Bob\n" << std::endl;
            }
        }
        else if (command == "accept") {
            std::string friendNickname;
            if (iss >> friendNickname) {
                acceptCall(friendNickname);
            }
            else {
                std::cout << "\nIncorrect command usage: accept <friend_nickname>" << std::endl;
                std::cout << "Parameters:  <friend_nickname> - nickname of the user who is calling you" << std::endl;
                std::cout << "Example:  accept Alice\n" << std::endl;
            }
        }
        else if (command == "decline") {
            std::string friendNickname;
            if (iss >> friendNickname) {
                declineCall(friendNickname);
            }
            else {
                std::cout << "\nIncorrect command usage: decline <friend_nickname>" << std::endl;
                std::cout << "Parameters:  <friend_nickname> - nickname of the user who is calling you" << std::endl;
                std::cout << "Example:  decline Bob\n" << std::endl;
            }
        }
        else if (command == "hangup") {
            hangUp();
        }
        else if (command == "mute") {
            std::string state;
            if (iss >> state) {
                setMute(state);
            }
            else {
                std::cout << "\nIncorrect command usage: mute <on/off>" << std::endl;
                std::cout << "Parameters:  <on> - mute microphone" << std::endl;
                std::cout << "            <off> - unmute microphone" << std::endl;
                std::cout << "Example:  mute on\n" << std::endl;
            }
        }
        else {
            std::cout << "\nUnknown command: " << command << "\n" << std::endl;
        }
    }

    void authorize(const std::string& nickname) {
        if (m_client->isAuthorized()) {
            std::cout << "Already authorized as: " << m_client->getNickname() << std::endl;
            return;
        }

        std::cout << "Authorizing as: " << nickname << "..." << std::endl;
        m_client->authorize(nickname);

        std::unique_lock<std::mutex> lock(m_consoleMutex);
        m_cv.wait(lock, [this] { return !m_client->isRunning() || m_client->isAuthorized() || m_client->getNickname().empty(); });
    }

    void callFriend(const std::string& friendNickname) {
        if (!m_client->isAuthorized()) {
            std::cout << "Please authorize first using 'auth <nickname>'" << std::endl;
            return;
        }

        if (friendNickname == m_client->getNickname()) {
            std::cout << "Cannot create call with yourself." << std::endl;
            return;
        }

        if (m_client->isInCall()) {
            std::cout << "Already in a call. Use 'hangup' to end current call first." << std::endl;
            return;
        }

        std::cout << "Calling " << friendNickname << "..." << std::endl;
        m_currentFriendNickname = friendNickname;
        m_client->createCall(friendNickname);

        std::unique_lock<std::mutex> lock(m_consoleMutex);
        m_cv.wait(lock, [this] { return !m_client->isRunning() || m_client->isInCall() || m_currentFriendNickname.empty(); });
    }

    void acceptCall(const std::string& friendNickname) {
        if (!m_client->isAuthorized()) {
            std::cout << "Please authorize first using 'auth <nickname>'" << std::endl;
            return;
        }

        if (m_client->isInCall()) {
            std::cout << "Already in a call. Use 'hangup' to end current call first." << std::endl;
            return;
        }

        bool incomingCallExist = m_client->acceptIncomingCall(friendNickname);
        if (incomingCallExist) {
            std::cout << "Accepted call from " << friendNickname << std::endl;
            m_currentFriendNickname = friendNickname;
        }
        else {
            std::cout << "No incoming call from " << friendNickname << std::endl;
        }
    }

    void declineCall(const std::string& friendNickname) {
        if (!m_client->isAuthorized()) {
            std::cout << "Please authorize first using 'auth <nickname>'" << std::endl;
            return;
        }

        bool incomingCallExist = m_client->declineIncomingCall(friendNickname);
        if (incomingCallExist) {
            std::cout << "Declined call from " << friendNickname << std::endl;
        }
        else {
            std::cout << "No incoming call from " << friendNickname << std::endl;
        }
    }

    void hangUp() {
        if (!m_client->isInCall()) {
            std::cout << "Not in a call" << std::endl;
            return;
        }

        m_client->endCall();
        m_currentFriendNickname.clear();

        std::cout << "Call ended" << std::endl;
        std::cout << "> ";
    }

    void setMute(const std::string& state) {
        if (!m_client->isAuthorized()) {
            std::cout << "Please authorize first using 'auth <nickname>'" << std::endl;
            return;
        }

        if (!m_client->isInCall()) {
            std::cout << "Not in a call" << std::endl;
            return;
        }

        if (state == "on") {
            m_client->mute(true);
            m_isMuted = true;
            std::cout << "Microphone muted" << std::endl;
        }
        else if (state == "off") {
            m_client->mute(false);
            m_isMuted = false;
            std::cout << "Microphone unmuted" << std::endl;
        }
        else {
            std::cout << "Invalid parameter. Use 'mute on' or 'mute off'" << std::endl;
        }
    }
};

int main() {
    setlocale(LC_ALL, "ru");

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