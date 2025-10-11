#include "hook.hpp"
#include "runtime/coroutine.hpp"
#include "runtime/scheduler.hpp"
#include "runtime/iomanager.hpp"
#include "system/fd_manager.hpp"
#include "core/macro.hpp"
#include "config/config.hpp"
#include <dlfcn.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/ioctl.h>

/*
整体工作原理：

    系统启动时调用hook_init()函数
    该函数使用dlsym(RTLD_NEXT, "sleep")找到系统原始的sleep函数地址，并保存到sleep_f变量中
    同样地，找到系统原始的usleep函数地址，并保存到usleep_f变量中
    后续当程序中调用sleep或usleep时，实际会调用框架提供的替代函数
    替代函数执行完自定义逻辑（如协程调度）后，通过sleep_f或usleep_f调用原始的系统函数
    这种技术叫做"函数拦截"或"API hooking"，允许框架在不修改用户代码的情况下，改变系统函数的行为，
    这对于实现协程调度特别有用——可以让协程在"睡眠"时主动让出执行权给其他协程，而不是阻塞整个线程。
*/

namespace sylar
{
    sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

    static sylar::ConfigVar<int>::ptr g_tcp_connect_timeout =
        sylar::Config::Lookup("tcp.connect.timeout", 5000, "tcp connect timeout");

    static thread_local bool t_hook_enable = false;

    // 定义需要hook的函数列表
#define HOOK_FUN(XX) \
    XX(sleep)        \
    XX(usleep)       \
    XX(nanosleep)    \
    XX(socket)       \
    XX(connect)      \
    XX(accept)       \
    XX(read)         \
    XX(readv)        \
    XX(recv)         \
    XX(recvfrom)     \
    XX(recvmsg)      \
    XX(write)        \
    XX(writev)       \
    XX(send)         \
    XX(sendto)       \
    XX(sendmsg)      \
    XX(close)        \
    XX(fcntl)        \
    XX(ioctl)        \
    XX(getsockopt)   \
    XX(setsockopt)

    // 初始化hook，保存原始函数地址
    void hook_init()
    {
        static bool is_inited = false;
        if (is_inited)
        {
            return;
        }
        // 获取原始函数地址
#define XX(name) name##_f = (name##_fun)dlsym(RTLD_NEXT, #name);
        HOOK_FUN(XX)
#undef XX
    }

    static uint64_t s_connect_timeout = -1;
    // 静态初始化器，确保程序启动时就完成hook初始化
    struct HookIniter
    {
        HookIniter()
        {
            hook_init();
            s_connect_timeout = g_tcp_connect_timeout->getValue();

            g_tcp_connect_timeout->addListener([](const int &old_value, const int &new_value)
                                               {
                                                SYLAR_LOG_INFO(g_logger)<<"tcp connect timeout changed from "
                                                                        <<old_value<<" to "<<new_value;
                                                s_connect_timeout = new_value; });
        }
    };

    static HookIniter s_hook_initer;

    bool is_hook_enable()
    {
        return t_hook_enable;
    }

    void set_hook_enable(bool flag)
    {
        t_hook_enable = flag;
    }

    struct timer_info
    {
        int cancelled = 0;
    };

    /**
     * @brief 执行带有协程支持的IO操作
     * @tparam OriginFun 原始函数类型
     * @tparam Args 函数参数包类型
     * @param fd 文件描述符
     * @param fun 原始系统调用函数指针
     * @param hook_fun_name 被hook的函数名称
     * @param event IO事件类型（READ/WRITE）
     * @param timeout_so 超时设置选项（SO_RCVTIMEO/SO_SNDTIMEO）
     * @param args 传递给原始函数的参数包
     * @return 返回IO操作结果，成功返回传输字节数，失败返回-1并设置errno
     *
     * 该函数是所有IO相关hook函数的核心实现，提供以下功能：
     * 1. 检查hook是否启用，未启用则直接调用原始函数
     * 2. 获取文件描述符上下文信息
     * 3. 处理非阻塞IO操作
     * 4. 在IO阻塞时将当前协程挂起，并注册相应的事件监听
     * 5. 支持超时控制
     */
    template <typename OriginFun, typename... Args>
    static ssize_t do_io(int fd, OriginFun fun, const char *hook_fun_name,
                         uint32_t event, int timeout_so, Args &&...args)
    {
        // 如果未启用hook，则直接调用原始函数
        if (!sylar::is_hook_enable())
        {
            return fun(fd, std::forward<Args>(args)...);
        }

        // 获取文件描述符上下文
        sylar::FdCtx::ptr ctx = sylar::FdMgr::GetInstance()->get(fd);
        if (!ctx)
        {
            return fun(fd, std::forward<Args>(args)...);
        }

        // 如果文件描述符已经关闭，设置错误码并返回
        if (ctx->isClose())
        {
            errno = EBADF;
            return -1;
        }

        // 如果不是套接字或者用户设置了非阻塞模式，则直接调用原始函数
        if (!ctx->isSocket() || ctx->getUserNonBlock())
        {
            return fun(fd, std::forward<Args>(args)...);
        }

        // 获取超时设置
        uint64_t to = ctx->getTimeout(timeout_so);
        std::shared_ptr<timer_info> tinfo(new timer_info);

    // 重试标签，用于IO操作被中断或需要重试的情况
    retry:
        // 尝试执行原始IO操作
        ssize_t n = fun(fd, std::forward<Args>(args)...);
        // 如果被信号中断，则重试
        while (n == -1 && errno == EINTR)
        {
            n = fun(fd, std::forward<Args>(args)...);
        }

        // 如果是因为缓冲区无数据/无法写入导致的阻塞
        if (n == -1 && errno == EAGAIN)
        {
            sylar::IOManager *iom = sylar::IOManager::GetThis();
            sylar::Timer::ptr timer;
            std::weak_ptr<timer_info> winfo(tinfo);

            // 如果设置了超时时间，则添加条件定时器
            if (to != (uint64_t)-1)
            {
                timer = iom->addConditionTimer(to, [winfo, fd, iom, event]()
                                               { 
                                            auto t = winfo.lock();
                                            if(!t||t->cancelled)
                                            {
                                                return;
                                            } 
                                            t->cancelled = ETIMEDOUT;
                                            iom->cancelEvent(fd,(sylar::IOManager::Event)event); }, winfo);
            }

            // 添加IO事件监听
            int rt = iom->addEvent(fd, (sylar::IOManager::Event)event);
            if (rt)
            {
                // 添加事件失败，记录日志并返回错误
                SYLAR_LOG_ERROR(g_logger) << hook_fun_name << " addEvent (" << fd << ", " << event << ")";
                if (timer)
                {
                    timer->cancel();
                }
                return -1;
            }
            else
            {
                // 成功添加事件，让出当前协程控制权
                sylar::Coroutine::YieldToHold();
                if (timer)
                {
                    timer->cancel();
                }
                // 如果定时器触发（超时），设置错误码并返回
                if (tinfo->cancelled)
                {
                    errno = tinfo->cancelled;
                    return -1;
                }
                // 重新尝试IO操作
                goto retry;
            }
        }
        return n;
    }

    extern "C"
    {
        // 定义函数指针
#define XX(name) name##_fun name##_f = nullptr;
        HOOK_FUN(XX)
#undef XX

        /**
         * @brief 重写的sleep函数，支持协程调度
         * @param seconds 要休眠的秒数
         * @return 返回0表示成功执行
         *
         * 当hook启用时，该函数会使用IOManager的定时器功能实现非阻塞式休眠：
         * 1. 获取当前IOManager实例
         * 2. 添加一个定时器，在指定时间后触发
         * 3. 定时器回调中将当前协程加入调度队列
         * 4. 让出当前协程的执行权
         * 5. 时间到达后协程会被重新调度执行
         */
        unsigned int sleep(unsigned int seconds)
        {
            if (!sylar::is_hook_enable())
            {
                return sleep_f(seconds);
            }

            sylar::IOManager *io_manager = sylar::IOManager::GetThis();
            if (!io_manager)
            {
                return sleep_f(seconds);
            }

            sylar::Coroutine::ptr co = sylar::Coroutine::GetThis();
            io_manager->addTimer(seconds * 1000, [io_manager, co]()
                                 { io_manager->schedule(co); });
            sylar::Coroutine::YieldToHold();

            return 0;
        }

        /**
         * @brief 重写的usleep函数，支持协程调度
         * @param usec 要休眠的微秒数
         * @return 返回0表示成功执行
         *
         * 当hook启用时，该函数会使用IOManager的定时器功能实现非阻塞式休眠：
         * 1. 获取当前IOManager实例
         * 2. 添加一个定时器，在指定时间后触发
         * 3. 定时器回调中将当前协程加入调度队列
         * 4. 让出当前协程的执行权
         * 5. 时间到达后协程会被重新调度执行
         */
        int usleep(useconds_t usec)
        {
            if (!sylar::is_hook_enable())
            {
                return usleep_f(usec);
            }

            sylar::IOManager *io_manager = sylar::IOManager::GetThis();
            if (!io_manager)
            {
                return usleep_f(usec);
            }

            sylar::Coroutine::ptr co = sylar::Coroutine::GetThis();
            io_manager->addTimer(usec / 1000, [io_manager, co]()
                                 { io_manager->schedule(co); });
            sylar::Coroutine::YieldToHold();

            return 0;
        }

        /**
         * @brief 重写的nanosleep函数，支持协程调度
         * @param req 要休眠的时间间隔
         * @param rem 如果函数被信号中断，返回剩余的时间间隔
         * @return 成功返回0，出错返回-1，并设置errno
         *
         * 当hook启用时，该函数会使用IOManager的定时器功能实现非阻塞式休眠：
         * 1. 获取当前IOManager实例
         * 2. 将timespec结构转换为毫秒数
         * 3. 添加一个定时器，在指定时间后触发
         * 4. 定时器回调中将当前协程加入调度队列
         * 5. 让出当前协程的执行权
         * 6. 时间到达后协程会被重新调度执行
         */
        int nanosleep(const struct timespec *req, struct timespec *rem)
        {
            if (!sylar::is_hook_enable())
            {
                return nanosleep_f(req, rem);
            }

            sylar::IOManager *io_manager = sylar::IOManager::GetThis();
            if (!io_manager)
            {
                return nanosleep_f(req, rem);
            }

            sylar::Coroutine::ptr co = sylar::Coroutine::GetThis();
            // 将纳秒转换为毫秒
            uint64_t timeout_ms = req->tv_sec * 1000 + req->tv_nsec / 1000 / 1000;
            io_manager->addTimer(timeout_ms, [io_manager, co]()
                                 { io_manager->schedule(co); });
            sylar::Coroutine::YieldToHold();

            return 0;
        }

        // socket相关函数将在后续实现
        int socket(int domain, int type, int protocol)
        {
            if (!sylar::is_hook_enable())
            {
                return socket_f(domain, type, protocol);
            }

            int fd = socket_f(domain, type, protocol);
            if (-1 == fd)
            {
                return fd;
            }
            sylar::FdMgr::GetInstance()->get(fd, true);
            return fd;
        }

        int connect_with_timeout(int fd, const struct sockaddr *addr, socklen_t addrlen, uint64_t timeout_ms)
        {
            if (!sylar::t_hook_enable)
            {
                return connect_f(fd, addr, addrlen);
            }
            sylar::FdCtx::ptr ctx = sylar::FdMgr::GetInstance()->get(fd);
            if (!ctx || ctx->isClose())
            {
                errno = EBADF;
                return -1;
            }

            if (!ctx->isSocket())
            {
                return connect_f(fd, addr, addrlen);
            }

            if (ctx->getUserNonBlock())
            {
                return connect_f(fd, addr, addrlen);
            }

            int n = connect_f(fd, addr, addrlen);
            if (n == 0)
            {
                return 0;
            }
            else if (n != -1 || errno != EINPROGRESS)
            {
                return n;
            }

            sylar::IOManager *iom = sylar::IOManager::GetThis();
            sylar::Timer::ptr timer;
            std::shared_ptr<timer_info> tinfo(new timer_info);
            std::weak_ptr<timer_info> winfo(tinfo);

            if (timeout_ms != (uint64_t)-1)
            {
                timer = iom->addConditionTimer(timeout_ms, [winfo, fd, iom]()
                                               {
                auto t = winfo.lock();
                if(!t || t->cancelled) {
                    return;
                }
                t->cancelled = ETIMEDOUT;
                iom->cancelEvent(fd, sylar::IOManager::WRITE); }, winfo);
            }

            int rt = iom->addEvent(fd, sylar::IOManager::WRITE);
            if (rt == 0)
            {
                sylar::Coroutine::YieldToHold();
                if (timer)
                {
                    timer->cancel();
                }
                if (tinfo->cancelled)
                {
                    errno = tinfo->cancelled;
                    return -1;
                }
            }
            else
            {
                if (timer)
                {
                    timer->cancel();
                }
                SYLAR_LOG_ERROR(g_logger) << "connect addEvent(" << fd << ", WRITE) error";
            }

            int error = 0;
            socklen_t len = sizeof(int);
            if (-1 == getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len))
            {
                return -1;
            }
            if (!error)
            {
                return 0;
            }
            else
            {
                errno = error;
                return -1;
            }
        }

        int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
        {
            return connect_with_timeout(sockfd, addr, addrlen, sylar::s_connect_timeout);
        }

        int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
        {
            int fd = do_io(sockfd, accept_f, "accept", sylar::IOManager::READ, SO_RCVTIMEO, addr, addrlen);
            if (fd >= 0)
            {
                sylar::FdMgr::GetInstance()->get(fd, true);
            }
            return fd;
        }

        ssize_t read(int fd, void *buf, size_t count)
        {
            return do_io(fd, read_f, "read", sylar::IOManager::READ, SO_RCVTIMEO, buf, count);
        }

        ssize_t readv(int fd, const struct iovec *iov, int iovcnt)
        {
            return do_io(fd, readv_f, "readv", sylar::IOManager::READ, SO_RCVTIMEO, iov, iovcnt);
        }

        ssize_t recv(int sockfd, void *buf, size_t len, int flags)
        {
            return do_io(sockfd, recv_f, "recv", sylar::IOManager::READ, SO_RCVTIMEO, buf, len, flags);
        }

        ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
                         struct sockaddr *src_addr, socklen_t *addrlen)
        {
            return do_io(sockfd, recvfrom_f, "recvfrom", sylar::IOManager::READ, SO_RCVTIMEO, buf, len, flags, src_addr, addrlen);
        }

        ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags)
        {
            return do_io(sockfd, recvmsg_f, "recvmsg", sylar::IOManager::READ, SO_RCVTIMEO, msg, flags);
        }

        ssize_t write(int fd, const void *buf, size_t count)
        {
            return do_io(fd, write_f, "write", sylar::IOManager::WRITE, SO_SNDTIMEO, buf, count);
        }

        ssize_t writev(int fd, const struct iovec *iov, int iovcnt)
        {
            return do_io(fd, writev_f, "writev", sylar::IOManager::WRITE, SO_SNDTIMEO, iov, iovcnt);
        }

        ssize_t send(int sockfd, const void *buf, size_t len, int flags)
        {
            return do_io(sockfd, send_f, "send", sylar::IOManager::WRITE, SO_SNDTIMEO, buf, len, flags);
        }

        ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,
                       const struct sockaddr *dest_addr, socklen_t addrlen)
        {
            return do_io(sockfd, sendto_f, "sendto", sylar::IOManager::WRITE, SO_SNDTIMEO, buf, len, flags, dest_addr, addrlen);
        }

        ssize_t sendmsg(int sockfd, const struct msghdr *msg, int flags)
        {
            return do_io(sockfd, sendmsg_f, "sendmsg", sylar::IOManager::WRITE, SO_SNDTIMEO, msg, flags);
        }

        int close(int fd)
        {
            if (!sylar::is_hook_enable())
            {
                return close_f(fd);
            }

            sylar::FdCtx::ptr ctx = sylar::FdMgr::GetInstance()->get(fd);
            if (ctx)
            {
                auto iom = sylar::IOManager::GetThis();
                if (iom)
                {
                    iom->cancelAll(fd);
                }
                sylar::FdMgr::GetInstance()->del(fd);
            }
            return close_f(fd);
        }

        // fcntl相关函数将在后续实现
        int fcntl(int fd, int cmd, ...)
        {
            va_list args;
            va_start(args, cmd);
            switch (cmd)
            {
            case F_SETFL:
            {
                int arg = va_arg(args, int);
                va_end(args);
                sylar::FdCtx::ptr ctx = sylar::FdMgr::GetInstance()->get(fd);
                if (!ctx || !ctx->isSocket() || ctx->isClose())
                {
                    return fcntl_f(fd, cmd, arg);
                }
                ctx->setUserNonBlock(arg & O_NONBLOCK);
                if (ctx->getSysNonBlock())
                {
                    arg |= O_NONBLOCK;
                }
                else
                {
                    arg &= ~O_NONBLOCK;
                }
                return fcntl_f(fd, cmd, arg);
            }
            break;
            case F_GETFL:
            {
                va_end(args);
                int arg = fcntl_f(fd, cmd);
                sylar::FdCtx::ptr ctx = sylar::FdMgr::GetInstance()->get(fd);
                if (!ctx || !ctx->isSocket() || ctx->isClose())
                {
                    return arg;
                }
                if (ctx->getUserNonBlock())
                {
                    return arg | O_NONBLOCK;
                }
                else
                {
                    return arg & ~O_NONBLOCK;
                }
            }
            break;
            case F_DUPFD:
            case F_DUPFD_CLOEXEC:
            case F_SETFD:
            case F_SETOWN:
            case F_SETSIG:
            case F_SETLEASE:
            case F_NOTIFY:
            case F_SETPIPE_SZ:
            {
                int arg = va_arg(args, int);
                va_end(args);
                return fcntl_f(fd, cmd, arg);
            }
            break;

            case F_GETFD:

            case F_GETOWN:
            case F_GETSIG:
            case F_GETLEASE:
            case F_GETPIPE_SZ:
            {
                va_end(args);
                return fcntl_f(fd, cmd);
            }
            break;

            case F_SETLK:
            case F_SETLKW:
            case F_GETLK:
            {
                struct flock *arg = va_arg(args, struct flock *);
                va_end(args);
                return fcntl_f(fd, cmd, arg);
            }
            break;

            case F_GETOWN_EX:
            case F_SETOWN_EX:
            {
                struct f_owner_exlock *arg = va_arg(args, struct f_owner_exlock *);
                va_end(args);
                return fcntl_f(fd, cmd, arg);
            }
            break;
            default:
            {
                va_end(args);
                return fcntl_f(fd, cmd);
            }
            }
        }
    }

    int ioctl(int fd, unsigned long request, ...)
    {
        va_list args;
        va_start(args, request);
        void *arg = va_arg(args, void *);
        va_end(args);

        if (FIONBIO == request)
        {
            bool user_nonblock = !!(*(int *)arg);
            sylar::FdCtx::ptr ctx = sylar::FdMgr::GetInstance()->get(fd);
            if (!ctx || !ctx->isSocket() || ctx->isClose())
            {
                return ioctl_f(fd, request, arg);
            }
            ctx->setSysNonBlock(user_nonblock);
        }
        return ioctl_f(fd, request, arg);
    }

    int getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen)
    {
        return getsockopt_f(sockfd, level, optname, optval, optlen);
    }

    int setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen)
    {
        if (!sylar::is_hook_enable())
        {
            return setsockopt_f(sockfd, level, optname, optval, optlen);
        }

        if (level == SOL_SOCKET)
        {
            if (optname == SO_RCVTIMEO || optname == SO_SNDTIMEO)
            {
                sylar::FdCtx::ptr ctx = sylar::FdMgr::GetInstance()->get(sockfd);
                if (ctx)
                {
                    const timeval *tv = (const timeval *)optval;
                    ctx->setTimeout(optname, tv->tv_sec * 1000 + tv->tv_usec / 1000);
                }
            }
        }
        return setsockopt_f(sockfd, level, optname, optval, optlen);
    }
}