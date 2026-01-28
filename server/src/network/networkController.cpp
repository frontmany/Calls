#include "networkController.h"
#include "utilities/logger.h"
#include "packet.h"

#include <utility>
#include <functional>

namespace server
{
    namespace network
    {
        NetworkController::NetworkController()
            : m_socket(m_context)
            , m_workGuard(asio::make_work_guard(m_context))
            , m_running(false)
            , m_nextPacketId(0U)
        {
        }

        NetworkController::~NetworkController() {
            stop();
        }

        bool NetworkController::init(const std::string& port,
            std::function<void(const unsigned char*, int, uint32_t, const asio::ip::udp::endpoint&)> onReceive)
        {
            m_onReceive = std::move(onReceive);

            try {
                std::error_code ec;
                asio::ip::udp::resolver resolver(m_context);
                auto endpoints = resolver.resolve(asio::ip::udp::v4(), "0.0.0.0", port, ec);
                if (ec) {
                    LOG_ERROR("Failed to resolve 0.0.0.0:{} - {}", port, ec.message());
                    return false;
                }
                if (endpoints.empty()) {
                    LOG_ERROR("No endpoints found for 0.0.0.0:{}", port);
                    return false;
                }
                m_serverEndpoint = *endpoints.begin();

                if (m_socket.is_open()) {
                    m_socket.close(ec);
                    ec.clear();
                }
                m_socket.open(asio::ip::udp::v4(), ec);
                if (ec) {
                    LOG_ERROR("Failed to open UDP socket: {}", ec.message());
                    return false;
                }
                m_socket.set_option(asio::socket_base::reuse_address(true), ec);
                if (ec) {
                    LOG_ERROR("Failed to set reuse_address on UDP socket: {}", ec.message());
                    return false;
                }
                m_socket.bind(m_serverEndpoint, ec);
                if (ec) {
                    LOG_ERROR("Failed to bind UDP socket: {}", ec.message());
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

                LOG_INFO("[UDP] Media server initialized on port {}", port);
                return true;
            }
            catch (const std::exception& e) {
                LOG_ERROR("NetworkController init error: {}", e.what());
                stop();
                return false;
            }
        }

        void NetworkController::run() {
            if (m_running.exchange(true))
                return;
            m_packetReceiver.start();
            try {
                m_context.run();
            }
            catch (const std::exception& e) {
                LOG_ERROR("asio io_context.run() threw: {}", e.what());
                throw;
            }
            catch (...) {
                LOG_ERROR("asio io_context.run() threw unknown exception");
                throw;
            }
        }

        void NetworkController::stop() {
            if (!m_running.exchange(false))
                return;
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

        bool NetworkController::isRunning() const {
            return m_running.load();
        }

        bool NetworkController::send(const std::vector<unsigned char>& data, uint32_t type, const asio::ip::udp::endpoint& endpoint) {
            if (type == 0 || type == 1) return false;
            Packet p;
            p.id = generateId();
            p.type = type;
            p.data = data;
            p.endpoint = endpoint;
            m_packetSender.send(p);
            return true;
        }

        bool NetworkController::send(std::vector<unsigned char>&& data, uint32_t type, const asio::ip::udp::endpoint& endpoint) {
            if (type == 0 || type == 1) return false;
            Packet p;
            p.id = generateId();
            p.type = type;
            p.data = std::move(data);
            p.endpoint = endpoint;
            m_packetSender.send(p);
            return true;
        }

        bool NetworkController::send(const unsigned char* data, int size, uint32_t type, const asio::ip::udp::endpoint& endpoint) {
            if (type == 0 || type == 1 || !data || size <= 0) return false;
            return send(std::vector<unsigned char>(data, data + size), type, endpoint);
        }

        uint64_t NetworkController::generateId() {
            return m_nextPacketId.fetch_add(1U, std::memory_order_relaxed);
        }
    }
}
