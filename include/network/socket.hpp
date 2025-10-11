#pragma once

#include "address.hpp"
#include "noncopyable.hpp"
#include <memory>

namespace sylar
{
    class Socket : public std::enable_shared_from_this<Socket>, Noncopyable
    {
    public:
        using ptr = std::shared_ptr<Socket>;
        using weak_ptr = std::weak_ptr<Socket>;

        Socket(int family, int type, int protocol);
        ~Socket();

        int64_t getSendTimeout() const;
        void setSendTimeout(int64_t v);

        int64_t getRecvTimeout() const;
        void setRecvTimeout(int64_t v);

        bool getOption(int level, int option, void *result, socklen_t *len) const;
        template <typename T>
        bool getOption(int level, int option, T &result)
        {
            socklen_t len = sizeof(T);
            return getOption(level, option, &result, &len);
        }

        bool setOption(int level, int option, const void *result, socklen_t len);
        template <typename T>
        bool setOption(int level, int option, const T &result)
        {
            return setOption(level, option, &result, sizeof(T));
        }

        Socket::ptr accept();

        bool bind(const Address::ptr &address);
        bool connect(const Address::ptr &address, uint64_t timeout_ms = -1);
        bool listen(int backlog = SOMAXCONN);
        bool close();

        int send(const void *buffer, size_t length, int flags = 0);
        int send(const iovec *vec, size_t size, int flags = 0);
        int sendTo(const void *buffer, size_t length, const Address::ptr &to, int flags = 0);
        int sendTo(const iovec *vec, size_t size, const Address::ptr &to, int flags = 0);

        int recv(void *buffer, size_t length, int flags = 0);
        int recv(iovec *vec, size_t size, int flags = 0);
        int recvFrom(void *buffer, size_t length, Address::ptr &from, int flags = 0);
        int recvFrom(iovec *vec, size_t size, Address::ptr &from, int flags = 0);

        Address::ptr getLocalAddress();
        Address::ptr getRemoteAddress();

        int getFamily() const;
        int getType() const;
        int getProtocol() const;
        int getSocket() const;

        bool isConnected() const;
        bool isValid() const;
        int getErrno() const;

        std::ostream &dump(std::ostream &os) const;

        bool cancelRead();
        bool cancelWrite();
        bool cancelAccept();
        bool cancelAll();

    private:
        bool init(int sockfd);
        void initSock();
        void newSock();

    private:
        int m_sock;                   /// socket文件描述符
        int m_family;                 /// 协议族
        int m_type;                   /// socket类型
        int m_protocol;               /// 协议类型
        bool m_isConnected;           /// 是否已连接标识
        Address::ptr m_localAddress;  /// 本地地址
        Address::ptr m_remoteAddress; /// 远程地址
    };
}
