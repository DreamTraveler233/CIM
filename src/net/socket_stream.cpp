#include "socket_stream.hpp"
#include "util.hpp"

namespace sylar
{
    SocketStream::SocketStream(Socket::ptr sock, bool owner)
        : m_socket(sock),
          m_owner(owner)
    {
    }

    SocketStream::~SocketStream()
    {
        if (m_owner && m_socket)
        {
            m_socket->close();
        }
    }

    bool SocketStream::isConnected() const
    {
        return m_socket && m_socket->isConnected();
    }

    /**
     * @brief 读取数据
     * @param[out] buffer 待接收数据的内存
     * @param[in] length 待接收数据的内存长度
     * @return
     *      @retval >0 返回实际接收到的数据长度
     *      @retval =0 socket被远端关闭
     *      @retval <0 socket错误
     */
    int SocketStream::read(void *buffer, size_t length)
    {
        if (!isConnected())
        {
            return -1;
        }
        return m_socket->recv(buffer, length);
    }

    /**
     * @brief 从ByteArray读取数据
     * @param[out] ba 接收数据的ByteArray
     * @param[in] length 待接收数据的内存长度
     * @return
     *      @retval >0 返回实际接收到的数据长度
     *      @retval =0 socket被远端关闭
     *      @retval <0 socket错误
     */
    int SocketStream::read(ByteArray::ptr ba, size_t length)
    {
        if (!isConnected())
        {
            return -1;
        }
        std::vector<iovec> iovs;
        ba->getWriteBuffers(iovs, length);
        int rt = m_socket->recv(&iovs[0], iovs.size());
        if (rt > 0)
        {
            ba->setPosition(ba->getPosition() + rt);
        }
        return rt;
    }

    /**
     * @brief 写入数据
     * @param[in] buffer 待发送数据的内存
     * @param[in] length 待发送数据的内存长度
     * @return
     *      @retval >0 返回实际发送的数据长度
     *      @retval =0 socket被远端关闭
     *      @retval <0 socket错误
     */
    int SocketStream::write(const void *buffer, size_t length)
    {
        if (!isConnected())
        {
            return -1;
        }
        return m_socket->send(buffer, length);
    }

    /**
     * @brief 从ByteArray写入数据
     * @param[in] ba 待发送数据的ByteArray
     * @param[in] length 待发送数据的内存长度
     * @return
     *      @retval >0 返回实际发送的数据长度
     *      @retval =0 socket被远端关闭
     *      @retval <0 socket错误
     */
    int SocketStream::write(ByteArray::ptr ba, size_t length)
    {
        if (!isConnected())
        {
            return -1;
        }
        std::vector<iovec> iovs;
        ba->getReadBuffers(iovs, length);
        int rt = m_socket->send(&iovs[0], iovs.size());
        if (rt > 0)
        {
            ba->setPosition(ba->getPosition() + rt);
        }
        return rt;
    }

    void SocketStream::close()
    {
        if (m_socket)
        {
            m_socket->close();
        }
    }

    Address::ptr SocketStream::getRemoteAddress()
    {
        if (m_socket)
        {
            return m_socket->getRemoteAddress();
        }
        return nullptr;
    }

    Address::ptr SocketStream::getLocalAddress()
    {
        if (m_socket)
        {
            return m_socket->getLocalAddress();
        }
        return nullptr;
    }

    std::string SocketStream::getRemoteAddressString()
    {
        auto addr = getRemoteAddress();
        if (addr)
        {
            return addr->toString();
        }
        return "";
    }

    std::string SocketStream::getLocalAddressString()
    {
        auto addr = getLocalAddress();
        if (addr)
        {
            return addr->toString();
        }
        return "";
    }

}