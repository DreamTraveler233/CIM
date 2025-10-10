#include "hook.hpp"
#include "coroutine.hpp"
#include "scheduler.hpp"
#include "iomanager.hpp"
#include "fdmanager.hpp"
#include "macro.hpp"
#include <dlfcn.h>

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

    // 静态初始化器，确保程序启动时就完成hook初始化
    struct HookIniter
    {
        HookIniter()
        {
            hook_init();
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
}

struct timer_info
{
    int cancelled;
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
                                            iom->cancelEvent(fd,event); }, winfo);
        }

        // 添加IO事件监听
        int rt = iom->addEvent(fd, event);
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

        // TODO: 实现socket函数的hook逻辑
        return socket_f(domain, type, protocol);
    }

    int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
    {
        if (!sylar::is_hook_enable())
        {
            return connect_f(sockfd, addr, addrlen);
        }

        // TODO: 实现connect函数的hook逻辑
        return connect_f(sockfd, addr, addrlen);
    }

    int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
    {
        if (!sylar::is_hook_enable())
        {
            return accept_f(sockfd, addr, addrlen);
        }

        // TODO: 实现accept函数的hook逻辑
        return accept_f(sockfd, addr, addrlen);
    }

    // read相关函数将在后续实现
    ssize_t read(int fd, void *buf, size_t count)
    {
        if (!sylar::is_hook_enable())
        {
            return read_f(fd, buf, count);
        }

        // TODO: 实现read函数的hook逻辑
        return read_f(fd, buf, count);
    }

    ssize_t readv(int fd, const struct iovec *iov, int iovcnt)
    {
        if (!sylar::is_hook_enable())
        {
            return readv_f(fd, iov, iovcnt);
        }

        // TODO: 实现readv函数的hook逻辑
        return readv_f(fd, iov, iovcnt);
    }

    ssize_t recv(int sockfd, void *buf, size_t len, int flags)
    {
        if (!sylar::is_hook_enable())
        {
            return recv_f(sockfd, buf, len, flags);
        }

        // TODO: 实现recv函数的hook逻辑
        return recv_f(sockfd, buf, len, flags);
    }

    ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
                     struct sockaddr *src_addr, socklen_t *addrlen)
    {
        if (!sylar::is_hook_enable())
        {
            return recvfrom_f(sockfd, buf, len, flags, src_addr, addrlen);
        }

        // TODO: 实现recvfrom函数的hook逻辑
        return recvfrom_f(sockfd, buf, len, flags, src_addr, addrlen);
    }

    ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags)
    {
        if (!sylar::is_hook_enable())
        {
            return recvmsg_f(sockfd, msg, flags);
        }

        // TODO: 实现recvmsg函数的hook逻辑
        return recvmsg_f(sockfd, msg, flags);
    }

    // write相关函数将在后续实现
    ssize_t write(int fd, const void *buf, size_t count)
    {
        if (!sylar::is_hook_enable())
        {
            return write_f(fd, buf, count);
        }

        // TODO: 实现write函数的hook逻辑
        return write_f(fd, buf, count);
    }

    ssize_t writev(int fd, const struct iovec *iov, int iovcnt)
    {
        if (!sylar::is_hook_enable())
        {
            return writev_f(fd, iov, iovcnt);
        }

        // TODO: 实现writev函数的hook逻辑
        return writev_f(fd, iov, iovcnt);
    }

    ssize_t send(int sockfd, const void *buf, size_t len, int flags)
    {
        if (!sylar::is_hook_enable())
        {
            return send_f(sockfd, buf, len, flags);
        }

        // TODO: 实现send函数的hook逻辑
        return send_f(sockfd, buf, len, flags);
    }

    ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,
                   const struct sockaddr *dest_addr, socklen_t addrlen)
    {
        if (!sylar::is_hook_enable())
        {
            return sendto_f(sockfd, buf, len, flags, dest_addr, addrlen);
        }

        // TODO: 实现sendto函数的hook逻辑
        return sendto_f(sockfd, buf, len, flags, dest_addr, addrlen);
    }

    ssize_t sendmsg(int sockfd, const struct msghdr *msg, int flags)
    {
        if (!sylar::is_hook_enable())
        {
            return sendmsg_f(sockfd, msg, flags);
        }

        // TODO: 实现sendmsg函数的hook逻辑
        return sendmsg_f(sockfd, msg, flags);
    }

    // close相关函数将在后续实现
    int close(int fd)
    {
        if (!sylar::is_hook_enable())
        {
            return close_f(fd);
        }

        // TODO: 实现close函数的hook逻辑
        return close_f(fd);
    }

    // fcntl相关函数将在后续实现
    int fcntl(int fd, int cmd, ...)
    {
        if (!sylar::is_hook_enable())
        {
            va_list args;
            va_start(args, cmd);
            int result = fcntl_f(fd, cmd, args);
            va_end(args);
            return result;
        }

        // TODO: 实现fcntl函数的hook逻辑
        va_list args;
        va_start(args, cmd);
        int result = fcntl_f(fd, cmd, args);
        va_end(args);
        return result;
    }

    int ioctl(int fd, unsigned long request, ...)
    {
        if (!sylar::is_hook_enable())
        {
            va_list args;
            va_start(args, request);
            int result = ioctl_f(fd, request, args);
            va_end(args);
            return result;
        }

        // TODO: 实现ioctl函数的hook逻辑
        va_list args;
        va_start(args, request);
        int result = ioctl_f(fd, request, args);
        va_end(args);
        return result;
    }

    int getsockopt(int sockfd, int level, int optname,
                   void *optval, socklen_t *optlen)
    {
        if (!sylar::is_hook_enable())
        {
            return getsockopt_f(sockfd, level, optname, optval, optlen);
        }

        // TODO: 实现getsockopt函数的hook逻辑
        return getsockopt_f(sockfd, level, optname, optval, optlen);
    }

    int setsockopt(int sockfd, int level, int optname,
                   const void *optval, socklen_t optlen)
    {
        if (!sylar::is_hook_enable())
        {
            return setsockopt_f(sockfd, level, optname, optval, optlen);
        }

        // TODO: 实现setsockopt函数的hook逻辑
        return setsockopt_f(sockfd, level, optname, optval, optlen);
    }
}