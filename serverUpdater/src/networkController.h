#pragma once
#include "safeDeque.h"
#include "packet.h"
#include "asio.hpp"

#include <unordered_set>


class Connection;

typedef std::shared_ptr<Connection> ConnectionPtr;

class NetworkController {
public:
    NetworkController(uint16_t port);
    ~NetworkController();
    bool start();
    void stop();

private:
    void waitForClientConnections();
    void processQueue(size_t maxPacketsCount = std::numeric_limits<unsigned long long>::max());
    void createConnection(uint64_t resolverID, asio::ip::tcp::socket socket);
    void onSendError(uint64_t resolverID, asio::ip::tcp::socket socket);

private:
    SafeDeque<Packet> m_queue;

    std::unordered_set<ConnectionPtr> m_setConnections;
    std::thread m_contextThread;

    asio::io_context m_context;
    asio::ip::tcp::acceptor m_asioAcceptor;
};
