#include "tcp_server.hpp"
#include "config.hpp"
#include "macro.hpp"

namespace sylar
{
    static auto g_logger = SYLAR_LOG_NAME("system");

    static sylar::ConfigVar<uint64_t>::ptr g_tcp_server_read_timeout =
        sylar::Config::Lookup("tcp_server.read_timeout", (uint64_t)(60 * 1000 * 2), "tcp server read timeout");

    static uint64_t s_tcp_server_read_timeout = g_tcp_server_read_timeout->getValue();

    TcpServer::TcpServer(IOManager *worker, IOManager *accept_workder)
        : m_worker(worker),
          m_acceptWorker(accept_workder),
          m_recvTimeout(s_tcp_server_read_timeout),
          m_name("sylar/1.0.0"),
          m_isStop(true)
    {
    }

    TcpServer::~TcpServer()
    {
        m_sockets.clear();
    }

    bool TcpServer::bind(const Address::ptr &addr)
    {
        std::vector<Address::ptr> addrs;
        std::vector<Address::ptr> fails;
        addrs.push_back(addr);
        return bind(addrs, fails);
    }

    bool TcpServer::bind(const std::vector<Address::ptr> &addrs, std::vector<Address::ptr> &fails)
    {
        for (auto &addr : addrs)
        {
            Socket::ptr socket = Socket::CreateTCP(addr);
            if (!socket->bind(addr))
            {
                SYLAR_LOG_ERROR(g_logger) << "bind fail errno="
                                          << errno << " errstr=" << strerror(errno)
                                          << " addr=[" << addr->toString() << "]";
                fails.push_back(addr);
                continue;
            }

            if (!socket->listen())
            {
                SYLAR_LOG_ERROR(g_logger) << "listen fail errno="
                                          << errno << " errstr=" << strerror(errno)
                                          << " addr=[" << addr->toString() << "]";
                fails.push_back(addr);
                continue;
            }

            m_sockets.push_back(socket);
        }

        if (!fails.empty())
        {
            m_sockets.clear();
            return false;
        }

        for (auto &socket : m_sockets)
        {
            SYLAR_LOG_INFO(g_logger) << " server bind success: " << *socket;
        }

        return true;
    }

    bool TcpServer::start()
    {
        if (!m_isStop)
        {
            return true;
        }
        m_isStop = false;
        for (auto &socket : m_sockets)
        {
            m_acceptWorker->schedule(std::bind(&TcpServer::startAccept,
                                               shared_from_this(), socket));
        }
        return true;
    }

    void TcpServer::stop()
    {
        m_isStop = true;
        // 捕获self，确保TcpServer对象在异步任务执行期间不会被销毁
        auto self = shared_from_this();
        m_acceptWorker->schedule([this, self]()
                                 {
        for(auto& socket : m_sockets) {
            socket->cancelAll();
        }
        m_sockets.clear(); });
    }

    uint64_t TcpServer::getRecvTimeout() const { return m_recvTimeout; }
    std::string TcpServer::getName() const { return m_name; }
    void TcpServer::setRecvTimeout(uint64_t time) { m_recvTimeout = time; }
    void TcpServer::setName(const std::string &name) { m_name = name; }
    bool TcpServer::isStop() const { return m_isStop; }
    void TcpServer::handleClient(Socket::ptr client)
    {
        SYLAR_LOG_INFO(g_logger) << "handleClient: " << *client;
    }
    void TcpServer::startAccept(Socket::ptr socket)
    {
        while (!m_isStop)
        {
            Socket::ptr client = socket->accept();
            if (client)
            {
                // 设置超时时间
                client->setRecvTimeout(m_recvTimeout);
                m_worker->schedule(std::bind(&TcpServer::handleClient,
                                             shared_from_this(), client));
            }
            else
            {
                SYLAR_LOG_ERROR(g_logger) << "accept errno=" << errno
                                          << " errstr=" << strerror(errno);
            }
        }
    }
}