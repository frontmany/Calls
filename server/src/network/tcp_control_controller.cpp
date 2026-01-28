#include "tcp_control_controller.h"
#include "utilities/logger.h"
#include "utilities/errorCodeForLog.h"

#include <chrono>

namespace server
{
    namespace network
    {
        using namespace std::chrono_literals;

        TcpControlController::TcpControlController(
            uint16_t port,
            OnControlPacket onPacket,
            OnDisconnect onDisconnect)
            : m_port(port)
            , m_acceptor(m_ctx, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port))
            , m_onPacket(std::move(onPacket))
            , m_onDisconnect(std::move(onDisconnect))
        {
        }

        TcpControlController::~TcpControlController() {
            stop();
        }

        void TcpControlController::start() {
            if (m_running.exchange(true))
                return;
            LOG_INFO("[TCP] Control server starting on port {}", m_port);
            m_ctxThread = std::thread([this]() { m_ctx.run(); });
            waitForClients();
            processQueue();
        }

        void TcpControlController::stop() {
            if (!m_running.exchange(false))
                return;
            m_acceptor.close();
            {
                std::lock_guard<std::mutex> lock(m_connMutex);
                for (auto& c : m_connections)
                    c->close();
                m_connections.clear();
            }
            m_ctx.stop();
            if (m_ctxThread.joinable())
                m_ctxThread.join();
            LOG_INFO("[TCP] Control server stopped");
        }

        bool TcpControlController::isRunning() const {
            return m_running.load();
        }

        void TcpControlController::waitForClients() {
            m_acceptor.async_accept([this](std::error_code ec, asio::ip::tcp::socket socket) {
                if (ec) {
                    if (ec != asio::error::operation_aborted)
                        LOG_ERROR("[TCP] Accept error: {}", server::utilities::errorCodeForLog(ec));
                    if (m_running.load())
                        waitForClients();
                    return;
                }
                auto ep = socket.remote_endpoint();
                LOG_INFO("[TCP] New connection from {}:{}", ep.address().to_string(), ep.port());
                createConnection(std::move(socket));

                if (m_running.load())
                    waitForClients();
            });
        }

        void TcpControlController::createConnection(asio::ip::tcp::socket socket) {
            TcpConnectionPtr conn = std::make_shared<TcpConnection>(
                m_ctx,
                std::move(socket),
                [this](OwnedTcpPacket&& p) { m_queue.push(std::move(p)); },
                [this](TcpConnectionPtr c) { handleDisconnect(c); });

            {
                std::lock_guard<std::mutex> lock(m_connMutex);
                m_connections.insert(conn);
                LOG_DEBUG("[TCP] Active connections: {}", m_connections.size());
            }
        }

        void TcpControlController::processQueue() {
            while (m_running.load()) {
                auto item = m_queue.pop_for(100ms);
                if (item && m_onPacket)
                    m_onPacket(std::move(*item));
            }
        }

        void TcpControlController::handleDisconnect(TcpConnectionPtr conn) {
            std::lock_guard<std::mutex> lock(m_connMutex);
            m_connections.erase(conn);
            LOG_INFO("[TCP] Client disconnected (active: {})", m_connections.size());
            if (m_onDisconnect)
                m_onDisconnect(conn);
        }
    }
}
