#include "socket.hpp"
#include "fd_manager.hpp"
#include "macro.hpp"
#include "hook.hpp"
#include <netinet/tcp.h>

namespace sylar
{
    static auto g_logger = loggerMgr::GetInstance()->getLogger("system");

    Socket::ptr Socket::CreateTCP(sylar::Address::ptr address)
    {
        Socket::ptr sock(new Socket(address->getFamily(), (int)Type::TCP, 0));
        return sock;
    }

    Socket::ptr Socket::CreateUDP(sylar::Address::ptr address)
    {
        Socket::ptr sock(new Socket(address->getFamily(), (int)Type::UDP, 0));
        return sock;
    }

    Socket::ptr Socket::CreateTCPSocket()
    {
        Socket::ptr sock(new Socket((int)Family::IPv4, (int)Type::TCP, 0));
        return sock;
    }

    Socket::ptr Socket::CreateUDPSocket()
    {
        Socket::ptr sock(new Socket((int)Family::IPv4, (int)Type::UDP, 0));
        return sock;
    }

    Socket::ptr Socket::CreateTCPSocket6()
    {
        Socket::ptr sock(new Socket((int)Family::IPv6, (int)Type::TCP, 0));
        return sock;
    }

    Socket::ptr Socket::CreateUDPSocket6()
    {
        Socket::ptr sock(new Socket((int)Family::IPv6, (int)Type::UDP, 0));
        return sock;
    }

    Socket::ptr Socket::CreateUnixTCPSocket()
    {
        Socket::ptr sock(new Socket((int)Family::UNIX, (int)Type::TCP, 0));
        return sock;
    }

    Socket::ptr Socket::CreateUnixUDPSocket()
    {
        Socket::ptr sock(new Socket((int)Family::UNIX, (int)Type::UDP, 0));
        return sock;
    }

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
        if (address == nullptr)
        {
            return false;
        }

        // 如果socket尚未创建，则先创建
        if (!isValid())
        {
            newSock();
            if (SYLAR_UNLIKELY(!isValid()))
            {
                return false;
            }
        }

        if (SYLAR_UNLIKELY(address->getFamily() != m_family))
        {
            SYLAR_LOG_ERROR(g_logger) << "bind socket.family(" << m_family << ") != address.family("
                                      << address->getFamily() << ") not equal, address=" << address->toString();
            return false;
        }

        // 调用系统bind函数绑定地址
        if (::bind(m_sock, address->getAddr(), address->getAddrLen()) == -1)
        {
            SYLAR_LOG_ERROR(g_logger) << "bind(" << m_sock << ") error errno=" << errno
                                      << " errstr=" << strerror(errno);
            return false;
        }

        // 保存本地地址信息
        getLocalAddress();
        return true;
    }

    bool Socket::connect(const Address::ptr &address, uint64_t timeout_ms)
    {
        if (address == nullptr)
        {
            return false;
        }

        // 如果socket尚未创建，则先创建
        if (!isValid())
        {
            newSock();
            if (SYLAR_UNLIKELY(!isValid()))
            {
                return false;
            }
        }

        if (SYLAR_UNLIKELY(address->getFamily() != m_family))
        {
            SYLAR_LOG_ERROR(g_logger) << "connect socket.family(" << m_family << ") != address.family("
                                      << address->getFamily() << ") not equal, address=" << address->toString();
            return false;
        }

        // 调用系统connect函数连接到指定地址
        if (timeout_ms == (uint64_t)-1)
        {
            if (::connect(m_sock, address->getAddr(), address->getAddrLen()) == -1)
            {
                SYLAR_LOG_ERROR(g_logger) << "connect(" << m_sock << ") error errno=" << errno
                                          << " errstr=" << strerror(errno);
                close(); // connect失败，关闭socket
                return false;
            }
        }
        else
        {
            if (connect_with_timeout(m_sock, address->getAddr(), address->getAddrLen(), timeout_ms) == -1)
            {
                SYLAR_LOG_ERROR(g_logger) << "connect(" << m_sock << ") error errno=" << errno
                                          << " errstr=" << strerror(errno);
                close(); // connect失败，关闭socket
                return false;
            }
        }

        // 保存远程地址信息
        m_isConnected = true;
        getRemoteAddress();
        getLocalAddress();
        return true;
    }

    bool Socket::listen(int backlog)
    {
        if (!isValid())
        {
            SYLAR_LOG_ERROR(g_logger) << "listen error: socket is invalid";
            return false;
        }

        if (::listen(m_sock, backlog) == -1)
        {
            SYLAR_LOG_ERROR(g_logger) << "listen(" << m_sock << ", " << backlog << ") error errno=" << errno
                                      << " errstr=" << strerror(errno);
            return false;
        }

        return true;
    }

    bool Socket::close()
    {
        if (!isConnected() && m_sock == -1)
        {
            return true;
        }
        m_isConnected = false;
        if (m_sock != -1)
        {
            ::close(m_sock);
            m_sock = -1;
        }
        return false;
    }

    int Socket::send(const void *buffer, size_t length, int flags)
    {
        if (!isConnected())
        {
            return -1;
        }
        int ret = ::send(m_sock, buffer, length, flags);
        if (ret == -1)
        {
            SYLAR_LOG_ERROR(g_logger) << "send(" << m_sock << ", " << length << ") error errno=" << errno
                                      << " errstr=" << strerror(errno);
        }
        return ret;
    }

    int Socket::send(const iovec *buffer, size_t size, int flags)
    {
        if (!isConnected())
        {
            return -1;
        }
        msghdr msg = {0};
        msg.msg_iov = const_cast<iovec *>(buffer);
        msg.msg_iovlen = size;
        int ret = ::sendmsg(m_sock, &msg, flags);
        if (ret == -1)
        {
            SYLAR_LOG_ERROR(g_logger) << "sendmsg(" << m_sock << ") error errno=" << errno
                                      << " errstr=" << strerror(errno);
        }
        return ret;
    }

    int Socket::sendTo(const void *buffer, size_t length, const Address::ptr &to, int flags)
    {
        if (!isValid() || to == nullptr)
        {
            return -1;
        }
        int ret = ::sendto(m_sock, buffer, length, flags, to->getAddr(), to->getAddrLen());
        if (ret == -1)
        {
            SYLAR_LOG_ERROR(g_logger) << "sendto(" << m_sock << ", " << length << ") error errno=" << errno
                                      << " errstr=" << strerror(errno);
        }
        return ret;
    }

    int Socket::sendTo(const iovec *buffer, size_t size, const Address::ptr &to, int flags)
    {
        if (!isValid() || to == nullptr)
        {
            return -1;
        }
        msghdr msg = {0};
        msg.msg_iov = const_cast<iovec *>(buffer);
        msg.msg_iovlen = size;
        msg.msg_name = to->getAddr();
        msg.msg_namelen = to->getAddrLen();
        int ret = ::sendmsg(m_sock, &msg, flags);
        if (ret == -1)
        {
            SYLAR_LOG_ERROR(g_logger) << "sendmsg(" << m_sock << ") error errno=" << errno
                                      << " errstr=" << strerror(errno);
        }
        return ret;
    }

    int Socket::recv(void *buffer, size_t length, int flags)
    {
        if (!isConnected())
        {
            return -1;
        }
        int ret = ::recv(m_sock, buffer, length, flags);
        if (ret == -1)
        {
            SYLAR_LOG_ERROR(g_logger) << "recv(" << m_sock << ", " << length << ") error errno=" << errno
                                      << " errstr=" << strerror(errno);
        }
        return ret;
    }

    int Socket::recv(iovec *buffer, size_t size, int flags)
    {
        if (!isConnected())
        {
            return -1;
        }
        msghdr msg = {0};
        msg.msg_iov = buffer;
        msg.msg_iovlen = size;
        int ret = ::recvmsg(m_sock, &msg, flags);
        if (ret == -1)
        {
            SYLAR_LOG_ERROR(g_logger) << "recvmsg(" << m_sock << ") error errno=" << errno
                                      << " errstr=" << strerror(errno);
        }
        return ret;
    }

    int Socket::recvFrom(void *buffer, size_t length, Address::ptr &from, int flags)
    {
        if (!isValid())
        {
            return -1;
        }
        socklen_t addrlen = from->getAddrLen();
        int ret = ::recvfrom(m_sock, buffer, length, flags, (sockaddr *)&from, &addrlen);
        if (ret == -1)
        {
            SYLAR_LOG_ERROR(g_logger) << "recvfrom(" << m_sock << ", " << length << ") error errno=" << errno
                                      << " errstr=" << strerror(errno);
        }
        return ret;
    }

    int Socket::recvFrom(iovec *buffer, size_t size, Address::ptr &from, int flags)
    {
        if (!isValid())
        {
            return -1;
        }
        msghdr msg = {0};
        msg.msg_name = from->getAddr();
        msg.msg_namelen = from->getAddrLen();
        msg.msg_iov = buffer;
        msg.msg_iovlen = size;
        int ret = ::recvmsg(m_sock, &msg, flags);
        if (ret == -1)
        {
            SYLAR_LOG_ERROR(g_logger) << "recvmsg(" << m_sock << ") error errno=" << errno
                                      << " errstr=" << strerror(errno);
        }
        return ret;
    }

    Address::ptr Socket::getLocalAddress()
    {
        if (m_localAddress)
        {
            return m_localAddress;
        }

        Address::ptr result;
        switch (m_family)
        {
        case AF_INET:
            result.reset(new IPv4Address());
            break;
        case AF_INET6:
            result.reset(new IPv6Address());
            break;
        case AF_UNIX:
            result.reset(new UnixAddress());
            break;
        default:
            result.reset(new UnknownAddress(m_family));
            break;
        }

        socklen_t addrlen = result->getAddrLen();
        if (getsockname(m_sock, result->getAddr(), &addrlen))
        {
            SYLAR_LOG_ERROR(g_logger) << "getsockname error sock=" << m_sock
                                      << " errno=" << errno << " errstr=" << strerror(errno);
            return Address::ptr(new UnknownAddress(m_family));
        }

        if (m_family == AF_UNIX)
        {
            UnixAddress::ptr address = std::dynamic_pointer_cast<UnixAddress>(result);
            address->setAddrLen(addrlen);
        }
        m_localAddress = result;
        return m_localAddress;
    }

    Address::ptr Socket::getRemoteAddress()
    {
        if (m_remoteAddress)
        {
            return m_remoteAddress;
        }

        Address::ptr result;
        switch (m_family)
        {
        case AF_INET:
            result.reset(new IPv4Address());
            break;
        case AF_INET6:
            result.reset(new IPv6Address());
            break;
        case AF_UNIX:
            result.reset(new UnixAddress());
            break;
        default:
            result.reset(new UnknownAddress(m_family));
            break;
        }

        socklen_t addrlen = result->getAddrLen();
        if (getpeername(m_sock, result->getAddr(), &addrlen))
        {
            SYLAR_LOG_ERROR(g_logger) << "getpeername error sock=" << m_sock
                                      << " errno=" << errno << " errstr=" << strerror(errno);
            return Address::ptr(new UnknownAddress(m_family));
        }

        if (m_family == AF_UNIX)
        {
            UnixAddress::ptr address = std::dynamic_pointer_cast<UnixAddress>(result);
            address->setAddrLen(addrlen);
        }
        m_remoteAddress = result;
        return m_remoteAddress;
    }

    int Socket::getFamily() const { return m_family; }
    int Socket::getType() const { return m_type; }
    int Socket::getProtocol() const { return m_protocol; }
    int Socket::getSocket() const { return m_sock; }
    bool Socket::isConnected() const { return m_isConnected; }
    bool Socket::isValid() const { return m_sock != -1; }
    int Socket::getErrno() const
    {
        int error = 0;
        socklen_t len = sizeof(error);
        if (!getOption(SOL_SOCKET, SO_ERROR, &error, &len))
        {
            return -1;
        }
        return error;
    }
    std::ostream &Socket::dump(std::ostream &os) const
    {
        os << "[Socket sock=" << m_sock
           << " isConnected=" << m_isConnected
           << " family=" << m_family
           << " type=" << m_type
           << " protocol=" << m_protocol;
        if (m_localAddress)
        {
            os << " localAddress=" << m_localAddress->toString();
        }
        if (m_remoteAddress)
        {
            os << " remoteAddress=" << m_remoteAddress->toString();
        }
        os << "]";
        return os;
    }
    bool Socket::cancelRead()
    {
        return IOManager::GetThis()->cancelEvent(m_sock, IOManager::READ);
    }
    bool Socket::cancelWrite()
    {
        return IOManager::GetThis()->cancelEvent(m_sock, IOManager::WRITE);
    }
    bool Socket::cancelAccept()
    {
        return IOManager::GetThis()->cancelEvent(m_sock, IOManager::READ);
    }
    bool Socket::cancelAll()
    {
        return IOManager::GetThis()->cancelAll(m_sock);
    }

    /**
     * @brief 初始化socket
     * @details 设置socket的选项，如SO_REUSEADDR和TCP_NODELAY
     */
    void Socket::initSock()
    {
        int val = 1;
        setOption(SOL_SOCKET, SO_REUSEADDR, val);
        if (m_type == SOCK_STREAM)
        {
            setOption(IPPROTO_TCP, TCP_NODELAY, val);
        }
    }

    /**
     * @brief 创建新的socket并初始化
     * @details 根据协议族、socket类型和协议创建一个新的socket，并进行初始化设置
     */
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