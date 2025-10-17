/**
 * @file socket.hpp
 * @brief Socket封装类
 * @author your_name
 * @date 2025-10-17
 *
 * 该文件定义了Socket类，用于封装网络套接字操作，包括创建、连接、监听、发送和接收数据等功能。
 * 支持TCP和UDP协议，IPv4和IPv6地址族，以及Unix域套接字。
 */

#pragma once

#include "address.hpp"
#include "noncopyable.hpp"
#include <memory>

namespace sylar
{
    /**
     * @brief Socket类，封装了套接字操作
     *
     * 该类提供了对套接字的高级封装，包括创建、连接、监听、发送和接收数据等操作。
     * 支持TCP和UDP协议，IPv4和IPv6地址族，以及Unix域套接字。
     * 使用智能指针管理Socket对象的生命周期。
     */
    class Socket : public std::enable_shared_from_this<Socket>, Noncopyable
    {
    public:
        /// 智能指针类型定义
        using ptr = std::shared_ptr<Socket>;
        using weak_ptr = std::weak_ptr<Socket>;

        /**
         * @brief Socket类型枚举
         */
        enum class Type
        {
            NONE = 0,          /// 未指定类型
            TCP = SOCK_STREAM, /// TCP类型
            UDP = SOCK_DGRAM   /// UDP类型
        };

        /**
         * @brief 协议族枚举
         */
        enum class Family
        {
            NONE = AF_UNSPEC, /// 未指定协议族
            IPv4 = AF_INET,   /// IPv4协议族
            IPv6 = AF_INET6,  /// IPv6协议族
            UNIX = AF_LOCAL   /// Unix域套接字协议族
        };

        /**
         * @brief 创建TCP套接字
         * @param[in] address 地址对象，根据地址类型确定创建IPv4还是IPv6套接字
         * @return Socket::ptr 创建的TCP套接字对象
         */
        static Socket::ptr CreateTCP(Address::ptr address);

        /**
         * @brief 创建UDP套接字
         * @param[in] address 地址对象，根据地址类型确定创建IPv4还是IPv6套接字
         * @return Socket::ptr 创建的UDP套接字对象
         */
        static Socket::ptr CreateUDP(Address::ptr address);

        /**
         * @brief 创建IPv4 TCP套接字
         * @return Socket::ptr 创建的IPv4 TCP套接字对象
         */
        static Socket::ptr CreateTCPSocket();

        /**
         * @brief 创建IPv4 UDP套接字
         * @return Socket::ptr 创建的IPv4 UDP套接字对象
         */
        static Socket::ptr CreateUDPSocket();

        /**
         * @brief 创建IPv6 TCP套接字
         * @return Socket::ptr 创建的IPv6 TCP套接字对象
         */
        static Socket::ptr CreateTCPSocket6();

        /**
         * @brief 创建IPv6 UDP套接字
         * @return Socket::ptr 创建的IPv6 UDP套接字对象
         */
        static Socket::ptr CreateUDPSocket6();

        /**
         * @brief 创建Unix域 TCP套接字
         * @return Socket::ptr 创建的Unix域 TCP套接字对象
         */
        static Socket::ptr CreateUnixTCPSocket();

        /**
         * @brief 创建Unix域 UDP套接字
         * @return Socket::ptr 创建的Unix域 UDP套接字对象
         */
        static Socket::ptr CreateUnixUDPSocket();

        /**
         * @brief 构造函数
         * @param[in] family 协议族
         * @param[in] type 套接字类型
         * @param[in] protocol 协议类型
         */
        Socket(int family, int type, int protocol);

        /**
         * @brief 析构函数
         */
        ~Socket();

        /**
         * @brief 获取发送超时时间
         * @return int64_t 发送超时时间(毫秒)
         */
        int64_t getSendTimeout() const;

        /**
         * @brief 设置发送超时时间
         * @param[in] v 发送超时时间(毫秒)
         */
        void setSendTimeout(int64_t v);

        /**
         * @brief 获取接收超时时间
         * @return int64_t 接收超时时间(毫秒)
         */
        int64_t getRecvTimeout() const;

        /**
         * @brief 设置接收超时时间
         * @param[in] v 接收超时时间(毫秒)
         */
        void setRecvTimeout(int64_t v);

        /**
         * @brief 获取套接字选项
         * @param[in] level 协议层次
         * @param[in] option 选项名称
         * @param[out] result 选项值
         * @param[inout] len 选项值长度
         * @return bool 获取成功返回true，失败返回false
         */
        bool getOption(int level, int option, void *result, socklen_t *len) const;

        /**
         * @brief 获取套接字选项模板函数
         * @tparam T 选项值类型
         * @param[in] level 协议层次
         * @param[in] option 选项名称
         * @param[out] result 选项值
         * @return bool 获取成功返回true，失败返回false
         */
        template <typename T>
        bool getOption(int level, int option, T &result)
        {
            socklen_t len = sizeof(T);
            return getOption(level, option, &result, &len);
        }

        /**
         * @brief 设置套接字选项
         * @param[in] level 协议层次
         * @param[in] option 选项名称
         * @param[in] result 选项值
         * @param[in] len 选项值长度
         * @return bool 设置成功返回true，失败返回false
         */
        bool setOption(int level, int option, const void *result, socklen_t len);

        /**
         * @brief 设置套接字选项模板函数
         * @tparam T 选项值类型
         * @param[in] level 协议层次
         * @param[in] option 选项名称
         * @param[in] result 选项值
         * @return bool 设置成功返回true，失败返回false
         */
        template <typename T>
        bool setOption(int level, int option, const T &result)
        {
            return setOption(level, option, &result, sizeof(T));
        }

        /**
         * @brief 接受连接
         * @return Socket::ptr 新的连接套接字对象，失败返回nullptr
         */
        Socket::ptr accept();

        /**
         * @brief 绑定地址
         * @param[in] address 要绑定的地址
         * @return bool 绑定成功返回true，失败返回false
         */
        bool bind(const Address::ptr &address);

        /**
         * @brief 连接指定地址
         * @param[in] address 要连接的地址
         * @param[in] timeout_ms 连接超时时间(毫秒)，默认为-1表示不超时
         * @return bool 连接成功返回true，失败返回false
         */
        bool connect(const Address::ptr &address, uint64_t timeout_ms = -1);

        /**
         * @brief 监听连接
         * @param[in] backlog 监听队列大小，默认为SOMAXCONN
         * @return bool 监听成功返回true，失败返回false
         */
        bool listen(int backlog = SOMAXCONN);

        /**
         * @brief 关闭套接字
         * @return bool 关闭成功返回true，失败返回false
         */
        bool close();

        /**
         * @brief 发送数据
         * @param[in] buffer 待发送数据缓冲区
         * @param[in] length 数据长度
         * @param[in] flags 发送标志
         * @return int 实际发送的字节数，失败返回-1
         */
        int send(const void *buffer, size_t length, int flags = 0);

        /**
         * @brief 发送分散数据
         * @param[in] vec 分散数据向量
         * @param[in] size 向量大小
         * @param[in] flags 发送标志
         * @return int 实际发送的字节数，失败返回-1
         */
        int send(const iovec *vec, size_t size, int flags = 0);

        /**
         * @brief 发送数据到指定地址
         * @param[in] buffer 待发送数据缓冲区
         * @param[in] length 数据长度
         * @param[in] to 目标地址
         * @param[in] flags 发送标志
         * @return int 实际发送的字节数，失败返回-1
         */
        int sendTo(const void *buffer, size_t length, const Address::ptr &to, int flags = 0);

        /**
         * @brief 发送分散数据到指定地址
         * @param[in] vec 分散数据向量
         * @param[in] size 向量大小
         * @param[in] to 目标地址
         * @param[in] flags 发送标志
         * @return int 实际发送的字节数，失败返回-1
         */
        int sendTo(const iovec *vec, size_t size, const Address::ptr &to, int flags = 0);

        /**
         * @brief 接收数据
         * @param[out] buffer 接收数据缓冲区
         * @param[in] length 缓冲区长度
         * @param[in] flags 接收标志
         * @return int 实际接收的字节数，失败返回-1
         */
        int recv(void *buffer, size_t length, int flags = 0);

        /**
         * @brief 接收分散数据
         * @param[out] vec 分散数据向量
         * @param[in] size 向量大小
         * @param[in] flags 接收标志
         * @return int 实际接收的字节数，失败返回-1
         */
        int recv(iovec *vec, size_t size, int flags = 0);

        /**
         * @brief 从指定地址接收数据
         * @param[out] buffer 接收数据缓冲区
         * @param[in] length 缓冲区长度
         * @param[out] from 发送方地址
         * @param[in] flags 接收标志
         * @return int 实际接收的字节数，失败返回-1
         */
        int recvFrom(void *buffer, size_t length, Address::ptr &from, int flags = 0);

        /**
         * @brief 从指定地址接收分散数据
         * @param[out] vec 分散数据向量
         * @param[in] size 向量大小
         * @param[out] from 发送方地址
         * @param[in] flags 接收标志
         * @return int 实际接收的字节数，失败返回-1
         */
        int recvFrom(iovec *vec, size_t size, Address::ptr &from, int flags = 0);

        /**
         * @brief 获取本地地址
         * @return Address::ptr 本地地址对象
         */
        Address::ptr getLocalAddress();

        /**
         * @brief 获取远程地址
         * @return Address::ptr 远程地址对象
         */
        Address::ptr getRemoteAddress();

        /**
         * @brief 获取协议族
         * @return int 协议族
         */
        int getFamily() const;

        /**
         * @brief 获取套接字类型
         * @return int 套接字类型
         */
        int getType() const;

        /**
         * @brief 获取协议类型
         * @return int 协议类型
         */
        int getProtocol() const;

        /**
         * @brief 获取套接字文件描述符
         * @return int 套接字文件描述符
         */
        int getSocket() const;

        /**
         * @brief 判断套接字是否已连接
         * @return bool 已连接返回true，否则返回false
         */
        bool isConnected() const;

        /**
         * @brief 判断套接字是否有效
         * @return bool 有效返回true，否则返回false
         */
        bool isValid() const;

        /**
         * @brief 获取套接字错误码
         * @return int 错误码
         */
        int getErrno() const;

        /**
         * @brief 输出套接字信息到流
         * @param[in] os 输出流
         * @return std::ostream& 输出流
         */
        std::ostream &dump(std::ostream &os) const;

        /**
         * @brief 取消读事件
         * @return bool 取消成功返回true，失败返回false
         */
        bool cancelRead();

        /**
         * @brief 取消写事件
         * @return bool 取消成功返回true，失败返回false
         */
        bool cancelWrite();

        /**
         * @brief 取消接受连接事件
         * @return bool 取消成功返回true，失败返回false
         */
        bool cancelAccept();

        /**
         * @brief 取消所有事件
         * @return bool 取消成功返回true，失败返回false
         */
        bool cancelAll();

    private:
        /**
         * @brief 初始化套接字
         * @param[in] sockfd 套接字文件描述符
         * @return bool 初始化成功返回true，失败返回false
         */
        bool init(int sockfd);

        /**
         * @brief 初始化套接字选项
         */
        void initSock();

        /**
         * @brief 创建新的套接字
         */
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

    std::ostream &operator<<(std::ostream &os, const Socket &socket);
}