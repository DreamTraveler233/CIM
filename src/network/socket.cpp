#include "socket.hpp"
#include "fd_manager.hpp"
#include "macro.hpp"
#include <netinet/tcp.h>

namespace sylar
{
    static auto g_logger = loggerMgr::GetInstance()->getLogger("system");

    Socket::Socket(int family, int type, int protocol)
        : m_sock(-1),
          m_family(family),
          m_type(type),
          m_protocol(protocol),
          m_isConnected(false)
    {
    }

    Socket::~Socket()
    {
        close();
    }

    int64_t Socket::getSendTimeout() const
    {
        FdCtx::ptr ctx = FdMgr::GetInstance()->get(m_sock);
        if (ctx)
        {
            return ctx->getTimeout(SO_SNDTIMEO);
        }
        return -1;
    }

    void Socket::setSendTimeout(int64_t v)
    {
        struct timeval tv = {v / 1000, (v % 1000) * 1000};
        setOption(SOL_SOCKET, SO_SNDTIMEO, tv);
    }

    int64_t Socket::getRecvTimeout() const
    {
        FdCtx::ptr ctx = FdMgr::GetInstance()->get(m_sock);
        if (ctx)
        {
            return ctx->getTimeout(SO_RCVTIMEO);
        }
        return -1;
    }

    void Socket::setRecvTimeout(int64_t v)
    {
        struct timeval tv = {v / 1000, (v % 1000) * 1000};
        setOption(SOL_SOCKET, SO_RCVTIMEO, tv);
    }

    bool Socket::getOption(int level, int option, void *result, socklen_t *len) const
    {
        if (getsockopt(m_sock, level, option, result, len) == -1)
        {
            SYLAR_LOG_DEBUG(g_logger) << "getOption sock = " << m_sock
                                      << " level = " << level
                                      << " option = " << option
                                      << " errno = " << errno
                                      << " errstr = " << strerror(errno);
            return false;
        }
        return true;
    }

    bool Socket::setOption(int level, int option, const void *result, socklen_t len)
    {
        if (setsockopt(m_sock, level, option, result, len) == -1)
        {
            SYLAR_LOG_DEBUG(g_logger) << "setOption sock = " << m_sock
                                      << " level = " << level
                                      << " option = " << option
                                      << " errno = " << errno
                                      << " errstr = " << strerror(errno);
            return false;
        }
        return true;
    }

    Socket::ptr Socket::accept()
    {
        Socket::ptr sock(new Socket(m_family, m_type, m_protocol));
        int new_sockfd = ::accept(m_sock, nullptr, nullptr);
        if (new_sockfd == -1)
        {
            SYLAR_LOG_ERROR(g_logger) << "accept(" << m_sock << ")" << " errno=" << errno << " " << strerror(errno);
            return nullptr;
        }
        if (sock->init(new_sockfd))
        {
            return sock;
        }
        return nullptr;
    }

    bool Socket::init(int sockfd)
    {
        FdCtx::ptr ctx = FdMgr::GetInstance()->get(sockfd);
        if (ctx && ctx->isSocket() && !ctx->isClose())
        {
            m_sock = sockfd;
            m_isConnected = true;
            initSock();
            getLocalAddress();
            getRemoteAddress();
            return true;
        }
        return false;
    }

    bool Socket::bind(const Address::ptr &address)
    {
        return false;
    }

    bool Socket::connect(const Address::ptr &address, uint64_t timeout_ms)
    {
        return false;
    }

    bool Socket::listen(int backlog)
    {
        return false;
    }

    bool Socket::close()
    {
        return false;
    }

    int Socket::send(const void *buffer, size_t length, int flags)
    {
        return 0;
    }

    int Socket::send(const iovec *vec, size_t size, int flags)
    {
        return 0;
    }

    int Socket::sendTo(const void *buffer, size_t length, const Address::ptr &to, int flags)
    {
        return 0;
    }

    int Socket::sendTo(const iovec *vec, size_t size, const Address::ptr &to, int flags)
    {
        return 0;
    }

    int Socket::recv(void *buffer, size_t length, int flags)
    {
        return 0;
    }

    int Socket::recv(iovec *vec, size_t size, int flags)
    {
        return 0;
    }

    int Socket::recvFrom(void *buffer, size_t length, Address::ptr &from, int flags)
    {
        return 0;
    }

    int Socket::recvFrom(iovec *vec, size_t size, Address::ptr &from, int flags)
    {
        return 0;
    }

    Address::ptr Socket::getLocalAddress()
    {
        return Address::ptr();
    }

    Address::ptr Socket::getRemoteAddress()
    {
        return Address::ptr();
    }

    int Socket::getFamily() const { return m_family; }
    int Socket::getType() const { return m_type; }
    int Socket::getProtocol() const { return m_protocol; }
    int Socket::getSocket() const { return m_sock; }
    bool Socket::isConnected() const { return m_isConnected; }
    bool Socket::isValid() const
    {
        return false;
    }
    int Socket::getErrno() const
    {
        return 0;
    }
    std::ostream &Socket::dump(std::ostream &os) const
    {
        // TODO: 在此处插入 return 语句
    }
    bool Socket::cancelRead()
    {
        return false;
    }
    bool Socket::cancelWrite()
    {
        return false;
    }
    bool Socket::cancelAccept()
    {
        return false;
    }
    bool Socket::cancelAll()
    {
        return false;
    }
    void Socket::initSock()
    {
        int val = 1;
        setOption(SOL_SOCKET, SO_REUSEADDR, val);
        if (m_type == SOCK_STREAM)
        {
            setOption(IPPROTO_TCP, TCP_NODELAY, val);
        }
    }
    void Socket::newSock()
    {
        m_sock = socket(m_family, m_type, m_protocol);
        if (SYLAR_LIKELY(m_sock != -1))
        {
            initSock();
        }
        else
        {
            SYLAR_LOG_ERROR(g_logger) << "socket(" << m_family << ", " << m_type << ", " << m_protocol
                                      << ") errno=" << errno << " errstr=" << strerror(errno);
        }
    }
}