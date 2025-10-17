#pragma once

#include "address.hpp"
#include "socket.hpp"
#include "iomanager.hpp"
#include "noncopyable.hpp"

namespace sylar
{
    class TcpServer : public std::enable_shared_from_this<TcpServer>, Noncopyable
    {
    public:
        using ptr = std::shared_ptr<TcpServer>;

        TcpServer(IOManager *worker = IOManager::GetThis(),
                  IOManager *accept_workder = IOManager::GetThis());
        virtual ~TcpServer();

        virtual bool bind(const Address::ptr &addr);
        virtual bool bind(const std::vector<Address::ptr> &addrs,
                          std::vector<Address::ptr> &fails);

        virtual bool start();
        virtual void stop();

        uint64_t getRecvTimeout() const;
        std::string getName() const;
        void setRecvTimeout(uint64_t time);
        void setName(const std::string &name);

        bool isStop() const;

    private:
        virtual void handleClient(Socket::ptr client);
        virtual void startAccept(Socket::ptr sock);

    private:
        std::vector<Socket::ptr> m_sockets;
        IOManager *m_worker;
        IOManager *m_acceptWorker;
        uint64_t m_recvTimeout; // 读超时时间
        std::string m_name;
        bool m_isStop;
    };
}