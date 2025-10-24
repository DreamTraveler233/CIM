#pragma once

#include "memory"
#include "lock.hpp"
#include "iomanager.hpp"
#include "singleton.hpp"
#include "noncopyable.hpp"

namespace sylar
{
    class FdCtx : public std::enable_shared_from_this<FdCtx>
    {
    public:
        using ptr = std::shared_ptr<FdCtx>;

        FdCtx(int fd);
        ~FdCtx();

        bool init();
        bool isInit() const;
        bool isSocket() const;
        bool isClose() const;
        bool close();

        void setUserNonBlock(bool v);
        bool getUserNonBlock() const;
        void setSysNonBlock(bool v);
        bool getSysNonBlock() const;

        void setTimeout(int type, uint64_t v);
        uint64_t getTimeout(int type) const;

    private:
        bool m_isInit : 1;       // 占用1个bit，表示对象是否已初始化
        bool m_isSocket : 1;     // 占用1个bit，表示是否为socket文件描述符
        bool m_sysNonBlock : 1;  // 占用1个bit，表示系统层面是否设置了非阻塞模式
        bool m_userNonBlock : 1; // 占用1个bit，表示用户是否设置了非阻塞模式
        bool m_isClosed : 1;     // 占用1个bit，表示文件描述符是否已关闭
        int m_fd;                // 文件描述符
        uint64_t m_recvTimeout;  // 接收超时时间
        uint64_t m_sendTimeout;  // 发送超时时间
    };

    class FdManager
    {
    public:
        using ptr = std::shared_ptr<FdManager>;
        using RWMutexType = RWMutex;

        FdManager();

        FdCtx::ptr get(int fd, bool auto_create = false);
        void del(int fd);

    private:
        std::vector<FdCtx::ptr> m_fdCtxs;
        RWMutexType m_mutex;
    };

    using FdMgr = Singleton<FdManager>;

    /**
     * @brief RAII文件描述符管理类
     * @details 自动管理文件描述符的生命周期，在对象析构时自动关闭文件描述符
     */
    class FileDescriptor : public Noncopyable
    {
    public:
        /**
         * @brief 构造函数
         * @param fd 文件描述符
         */
        explicit FileDescriptor(int fd = -1);

        /**
         * @brief 析构函数，自动关闭文件描述符
         */
        ~FileDescriptor();

        /**
         * @brief 获取文件描述符
         * @return int 文件描述符
         */
        int get() const;

        /**
         * @brief 重置文件描述符
         * @param fd 新的文件描述符
         */
        void reset(int fd = -1);

        /**
         * @brief 释放文件描述符所有权
         * @return int 文件描述符
         */
        int release();

        /**
         * @brief 检查文件描述符是否有效
         * @return bool 文件描述符是否有效
         */
        bool isValid() const;

        // 支持移动
        FileDescriptor(FileDescriptor &&other) noexcept;
        FileDescriptor &operator=(FileDescriptor &&other) noexcept;

    private:
        int m_fd;
    };
}