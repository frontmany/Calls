#pragma once
#include <atomic>
#include <functional>
#include <string>
#include <vector>

#include "packetTypes.h"
#include "packetSender.h"
#include "packetReceiver.h"

#include "asio.hpp"

namespace calls {

    class NetworkController {
    public:
        NetworkController();
        ~NetworkController();

        bool init(const std::string& host,
            const std::string& port,
            std::function<void(const unsigned char*, int, PacketType)> onReceiveCallback,
            std::function<void()> onErrorCallback);

        void run();
        void stop();
        bool isRunning() const;
        void send(const std::vector<unsigned char>& data, PacketType type);
        void send(std::vector<unsigned char>&& data, PacketType type);
        void send(PacketType type);

    private:
        uint64_t generateId();

    private:
        asio::io_context m_context;
        asio::ip::udp::socket m_socket;
        asio::ip::udp::endpoint m_serverEndpoint;
        asio::executor_work_guard<asio::io_context::executor_type> m_workGuard;
        std::thread m_asioThread;

        PacketReceiver m_packetReceiver;
        PacketSender m_packetSender;

        std::atomic<bool> m_running;
        std::atomic<uint64_t> m_nextPacketId;

        std::function<void(const unsigned char*, int, PacketType)> m_onReceiveCallback;
        std::function<void()> m_onErrorCallback;
    };

}