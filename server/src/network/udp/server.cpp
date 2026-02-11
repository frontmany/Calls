#include "server.h"
#include "utilities/logger.h"
#include "utilities/errorCodeForLog.h"
#include "packet.h"

#include <utility>
#include <functional>
#include <chrono>

using namespace std::chrono_literals;

namespace server::network::udp
{
    Server::Server()
        : m_socket(m_context)
        , m_workGuard(asio::make_work_guard(m_context))
        , m_running(false)
        , m_nextPacketId(0U)
    {
    }

    Server::~Server() {
        stop();
    }

    bool Server::init(const std::string& port,
        std::function<void(const unsigned char*, int, uint32_t, const asio::ip::udp::endpoint&)> onReceive)
    {
        m_port = port;
        m_onReceive = std::move(onReceive);
        return reinit();
    }

    bool Server::reinit() {
        try {
            std::error_code ec;
            asio::ip::udp::resolver resolver(m_context);
            auto endpoints = resolver.resolve(asio::ip::udp::v4(), "0.0.0.0", m_port, ec);
            if (ec) {
                LOG_ERROR("Failed to resolve 0.0.0.0:{} - {}", m_port, server::utilities::errorCodeForLog(ec));
                return false;
            }
            if (endpoints.empty()) {
                LOG_ERROR("No endpoints found for 0.0.0.0:{}", m_port);
                return false;
            }
            m_serverEndpoint = *endpoints.begin();

            if (m_socket.is_open()) {
                m_socket.close(ec);
                ec.clear();
            }
            m_socket.open(asio::ip::udp::v4(), ec);
            if (ec) {
                LOG_ERROR("Failed to open UDP socket: {}", server::utilities::errorCodeForLog(ec));
                return false;
            }
            m_socket.set_option(asio::socket_base::reuse_address(true), ec);
            if (ec) {
                LOG_ERROR("Failed to set reuse_address on UDP socket: {}", server::utilities::errorCodeForLog(ec));
                return false;
            }
            m_socket.bind(m_serverEndpoint, ec);
            if (ec) {
                LOG_ERROR("Failed to bind UDP socket: {}", server::utilities::errorCodeForLog(ec));
                return false;
            }

            std::function<void()> errorHandler = []() {
                LOG_ERROR("[UDP] Packet send/receive error");
            };
            auto pingNoOp = [](uint32_t, const asio::ip::udp::endpoint&) {};

            if (!m_packetReceiver.init(m_socket, m_onReceive, errorHandler, pingNoOp)) {
                LOG_ERROR("Failed to initialize packet receiver");
                return false;
            }
            m_packetSender.init(m_socket, errorHandler);

            LOG_INFO("[UDP] Media server initialized on port {}", m_port);
            return true;
        }
        catch (const std::exception& e) {
            LOG_ERROR("Server init error: {}", e.what());
            return false;
        }
    }

    void Server::start() {
        if (m_running.exchange(true))
            return;
        LOG_INFO("[UDP] Media server starting on port {}", m_port);
        m_ctxThread = std::thread([this]() {
            while (m_running.load()) {
                run();
                if (!m_running.load()) break;

                LOG_WARN("[UDP] io_context stopped unexpectedly, reinitializing in 1s...");
                std::this_thread::sleep_for(1s);

                stop_internal();
                if (!reinit()) {
                    LOG_ERROR("[UDP] Reinitialization failed, retrying in 3s...");
                    std::this_thread::sleep_for(3s);
                }
            }
        });
    }

    void Server::run() {
        m_packetReceiver.start();
        try {
            m_context.run();
        }
        catch (const std::exception& e) {
            LOG_ERROR("[UDP] io_context exception: {}", e.what());
        }
        catch (...) {
            LOG_ERROR("[UDP] io_context unknown exception");
        }
    }

    void Server::stop() {
        if (!m_running.exchange(false))
            return;
        stop_internal();
        if (m_ctxThread.joinable())
            m_ctxThread.join();
        LOG_INFO("[UDP] Media server stopped");
    }

    void Server::stop_internal() {
        m_packetSender.stop();
        m_packetReceiver.stop();
        std::error_code ec;
        if (m_socket.is_open()) {
            m_socket.cancel(ec);
            m_socket.close(ec);
        }
        m_workGuard.reset();
        m_context.stop();
        m_context.restart();
    }

    bool Server::isRunning() const {
        return m_running.load();
    }

    bool Server::send(const std::vector<unsigned char>& data, uint32_t type, const asio::ip::udp::endpoint& endpoint) {
        if (type == 0 || type == 1) return false;
        Packet p;
        p.id = generateId();
        p.type = type;
        p.data = data;
        p.endpoint = endpoint;
        m_packetSender.send(p);
        return true;
    }

    bool Server::send(std::vector<unsigned char>&& data, uint32_t type, const asio::ip::udp::endpoint& endpoint) {
        if (type == 0 || type == 1) return false;
        Packet p;
        p.id = generateId();
        p.type = type;
        p.data = std::move(data);
        p.endpoint = endpoint;
        m_packetSender.send(p);
        return true;
    }

    bool Server::send(const unsigned char* data, int size, uint32_t type, const asio::ip::udp::endpoint& endpoint) {
        if (type == 0 || type == 1 || !data || size <= 0) return false;
        return send(std::vector<unsigned char>(data, data + size), type, endpoint);
    }

    uint64_t Server::generateId() {
        return m_nextPacketId.fetch_add(1U, std::memory_order_relaxed);
    }
}
