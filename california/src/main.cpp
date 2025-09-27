#include <QApplication>
#include "mainWindow.h"

// Qt

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    std::unique_ptr<MainWindow> mainWindow = std::make_unique<MainWindow>(nullptr, "192.168.1.40", "8081");
    mainWindow->showMaximized();
    app.exec();


    return 0;
}

/*
#if defined(_WIN32)
#define _WIN32_WINNT 0x0A00 
#endif

#include <iostream>
#include <string>
#include <sstream>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>

class ConsoleApp {
private:
    std::mutex m_consoleMutex;
    std::condition_variable m_cv;
    std::string m_currentFriendNickname;
    bool m_isMuted = false;
    int m_inputVolume = 100;
    int m_outputVolume = 100;
    bool m_initialized = false;

public:
    ConsoleApp() {
        auto authCallback = [this](calls::Result result) {
            std::lock_guard<std::mutex> lock(m_consoleMutex);
            switch (result) {
            case calls::Result::SUCCESS:
                std::cout << "Authorization successful!" << std::endl;
                break;
            case calls::Result::FAIL:
                std::cout << "Authorization failed!" << std::endl;
                break;
            case calls::Result::TIMEOUT:
                std::cout << "Authorization timeout!" << std::endl;
                break;
            default:
                std::cout << "Authorization unknown result!" << std::endl;
                break;
            }
            m_cv.notify_one();
            };

        auto createCallCallback = [this](calls::Result result) {
            std::lock_guard<std::mutex> lock(m_consoleMutex);
            switch (result) {
            case calls::Result::SUCCESS:
                std::cout << "Call accepted by " << m_currentFriendNickname << "!" << std::endl;
                break;
            case calls::Result::FAIL:
                std::cout << "Call declined by " << m_currentFriendNickname << std::endl;
                m_currentFriendNickname.clear();
                break;
            case calls::Result::TIMEOUT:
                std::cout << "Call timeout!" << std::endl;
                m_currentFriendNickname.clear();
                break;
            default:
                std::cout << "Call result unknown: " << static_cast<int>(result) << std::endl;
                m_currentFriendNickname.clear();
                break;
            }
            m_cv.notify_one();
            };

        auto incomingCallCallback = [this](const std::string& friendNickName) {
            std::lock_guard<std::mutex> lock(m_consoleMutex);
            std::cout << "\nIncoming call from: " << friendNickName << std::endl;
            std::cout << "Type 'accept " << friendNickName << "' to accept or 'decline " << friendNickName << "' to decline" << std::endl;
            };

        auto incomingCallExpiredCallback = [this](const std::string& friendNickname) {
            std::lock_guard<std::mutex> lock(m_consoleMutex);
            std::cout << "Incoming call from " << friendNickname << " expired" << std::endl;
            };

        auto simultaneousCallingCallback = [this](const std::string& friendNickname) {
            std::lock_guard<std::mutex> lock(m_consoleMutex);
            std::cout << "Simultaneous calling detected from " << friendNickname << std::endl;
            };

        auto hangUpCallback = [this]() {
            std::lock_guard<std::mutex> lock(m_consoleMutex);
            std::cout << "Call ended" << std::endl;
            m_currentFriendNickname.clear();
            };

        auto networkErrorCallback = [this]() {
            std::lock_guard<std::mutex> lock(m_consoleMutex);
            std::cout << "Network error occurred!" << std::endl;
            m_cv.notify_one();
            };

        // Initialize the client
        // 92.255.165.77
        // 192.168.1.40 
        // 192.168.1.48
        calls::init(
            "192.168.1.40",
            "8081",
            std::move(authCallback),
            std::move(createCallCallback),
            std::move(incomingCallCallback),
            std::move(incomingCallExpiredCallback),
            std::move(simultaneousCallingCallback),
            std::move(hangUpCallback),
            std::move(networkErrorCallback)
        );

        m_initialized = true;

        // Start the client in a separate thread
        std::thread clientThread([]() {
            calls::run();
            });
        clientThread.detach();
    }

    void run() {
        if (!m_initialized) {
            std::cout << "Failed to initialize client!" << std::endl;
            return;
        }

        std::cout << "auth      <nickname>  - Authorize with nickname" << std::endl;
        std::cout << "call      <friend>    - Call a friend" << std::endl;
        std::cout << "accept    <friend>    - Accept incoming call" << std::endl;
        std::cout << "decline   <friend>    - Decline incoming call" << std::endl;
        std::cout << "hangup                - End current call" << std::endl;
        std::cout << "mute      <on/off>    - Mute/unmute microphone" << std::endl;
        std::cout << "mic       <0-100>     - Set microphone volume" << std::endl;
        std::cout << "headset   <0-100>     - Set headset volume" << std::endl;
        std::cout << "refresh               - Refresh audio devices (if changed)" << std::endl;
        std::cout << "status                - Show current status" << std::endl;
        std::cout << "quit                  - Exit application" << std::endl;
        std::cout << std::endl;

        std::string input;
        while (calls::isRunning()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(600));
            std::cout << "> ";
            std::getline(std::cin, input);

            if (input.empty()) continue;

            processCommand(input);
        }

        processCommand("quit");
    }

private:
    void processCommand(const std::string& input) {
        std::istringstream iss(input);
        std::string command;
        iss >> command;

        if (command == "quit" || command == "exit") {
            calls::stop();
            std::cout << "Goodbye!" << std::endl;
            return;
        }
        else if (command == "auth") {
            std::string nickname;
            if (iss >> nickname) {
                authorize(nickname);
            }
            else {
                std::cout << "\nIncorrect command usage: auth <nickname>" << std::endl;
                std::cout << "Parameters:  <nickname> - your unique identifier (3-20 characters, letters and numbers only)" << std::endl;
                std::cout << "Example:  auth Alice123\n" << std::endl;
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
        else if (command == "mic") {
            int volume;
            if (iss >> volume) {
                setInputVolume(volume);
            }
            else {
                std::cout << "\nIncorrect command usage: mic <0-100>" << std::endl;
                std::cout << "Parameters:  <0-100> - input volume level" << std::endl;
                std::cout << "Example:  mic 80\n" << std::endl;
            }
        }
        else if (command == "headset") {
            int volume;
            if (iss >> volume) {
                setOutputVolume(volume);
            }
            else {
                std::cout << "\nIncorrect command usage: headset <0-100>" << std::endl;
                std::cout << "Parameters:  <0-100> - output volume level" << std::endl;
                std::cout << "Example:  headset 90\n" << std::endl;
            }
        }
        else if (command == "refresh") {
            refreshAudioDevices();
        }
        else if (command == "status") {
            showStatus();
        }
        else {
            std::cout << "\nUnknown command: " << command << "\n" << std::endl;
        }
    }

    void authorize(const std::string& nickname) {
        if (isAuthorized()) {
            std::cout << "Already authorized as: " << calls::getNickname() << std::endl;
            return;
        }

        std::cout << "Authorizing as: " << nickname << "..." << std::endl;
        calls::authorize(nickname);

        std::unique_lock<std::mutex> lock(m_consoleMutex);
        m_cv.wait(lock, [this] {
            return !calls::isRunning() || isAuthorized() || calls::getNickname().empty();
            });
    }

    void callFriend(const std::string& friendNickname) {
        if (!isAuthorized()) {
            std::cout << "Please authorize first using 'auth <nickname>'" << std::endl;
            return;
        }

        if (friendNickname == calls::getNickname()) {
            std::cout << "Cannot create call with yourself." << std::endl;
            return;
        }

        if (isInCall()) {
            std::cout << "Already in a call. Use 'hangup' to end current call first." << std::endl;
            return;
        }

        std::cout << "Calling " << friendNickname << "..." << std::endl;
        m_currentFriendNickname = friendNickname;
        calls::createCall(friendNickname);

        std::unique_lock<std::mutex> lock(m_consoleMutex);
        m_cv.wait(lock, [this] {
            return !calls::isRunning() || isInCall() || m_currentFriendNickname.empty();
            });
    }

    void acceptCall(const std::string& friendNickname) {
        if (!isAuthorized()) {
            std::cout << "Please authorize first using 'auth <nickname>'" << std::endl;
            return;
        }

        if (isInCall()) {
            std::cout << "Already in a call. Use 'hangup' to end current call first." << std::endl;
            return;
        }

        bool success = calls::acceptCall(friendNickname);
        if (success) {
            std::cout << "Accepted call from " << friendNickname << std::endl;
            m_currentFriendNickname = friendNickname;
        }
        else {
            std::cout << "Failed to accept call from " << friendNickname << std::endl;
        }
    }

    void declineCall(const std::string& friendNickname) {
        if (!isAuthorized()) {
            std::cout << "Please authorize first using 'auth <nickname>'" << std::endl;
            return;
        }

        bool success = calls::declineCall(friendNickname);
        if (success) {
            std::cout << "Declined call from " << friendNickname << std::endl;
        }
        else {
            std::cout << "Failed to decline call from " << friendNickname << std::endl;
        }
    }

    void hangUp() {
        if (!isInCall()) {
            std::cout << "Not in a call" << std::endl;
            return;
        }

        calls::endCall();
        m_currentFriendNickname.clear();
        std::cout << "Call ended" << std::endl;
    }

    void setMute(const std::string& state) {
        if (!isAuthorized()) {
            std::cout << "Please authorize first using 'auth <nickname>'" << std::endl;
            return;
        }

        if (!isInCall()) {
            std::cout << "Not in a call" << std::endl;
            return;
        }

        if (state == "on") {
            calls::mute(true);
            m_isMuted = true;
            std::cout << "Microphone muted" << std::endl;
        }
        else if (state == "off") {
            calls::mute(false);
            m_isMuted = false;
            std::cout << "Microphone unmuted" << std::endl;
        }
        else {
            std::cout << "Invalid parameter. Use 'mute on' or 'mute off'" << std::endl;
        }
    }

    void setInputVolume(int volume) {
        if (volume < 0 || volume > 100) {
            std::cout << "Volume must be between 0 and 100" << std::endl;
            return;
        }

        calls::setInputVolume(volume);
        m_inputVolume = volume;
        std::cout << "Input volume set to: " << volume << std::endl;
    }

    void setOutputVolume(int volume) {
        if (volume < 0 || volume > 100) {
            std::cout << "Volume must be between 0 and 100" << std::endl;
            return;
        }

        calls::setOutputVolume(volume);
        m_outputVolume = volume;
        std::cout << "Output volume set to: " << volume << std::endl;
    }

    void refreshAudioDevices() {
        if (!isAuthorized()) {
            std::cout << "Please authorize first using 'auth <nickname>'" << std::endl;
            return;
        }

        calls::refreshAudioDevices();
        std::cout << "Audio devices refreshed" << std::endl;
    }

    void showStatus() {
        std::cout << "Status: " << stateToString(calls::getStatus()) << std::endl;
        std::cout << "Nickname: " << (calls::getNickname().empty() ? "Not authorized" : calls::getNickname()) << std::endl;
        std::cout << "Muted: " << (calls::isMuted() ? "Yes" : "No") << std::endl;
        std::cout << "Input volume: " << calls::getInputVolume() << std::endl;
        std::cout << "Output volume: " << calls::getOutputVolume() << std::endl;
        if (isInCall()) {
            std::cout << "In call with: " << calls::getNicknameWhomCalling() << std::endl;
        }
    }

    bool isAuthorized() const {
        return calls::getStatus() == calls::State::FREE ||
            calls::getStatus() == calls::State::CALLING ||
            calls::getStatus() == calls::State::BUSY;
    }

    bool isInCall() const {
        return calls::getStatus() == calls::State::BUSY;
    }

    std::string stateToString(calls::State state) const {
        switch (state) {
        case calls::State::UNAUTHORIZED: return "UNAUTHORIZED";
        case calls::State::FREE: return "FREE";
        case calls::State::CALLING: return "CALLING";
        case calls::State::BUSY: return "BUSY";
        default: return "UNKNOWN";
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
*/