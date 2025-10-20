#include "iomanager.hpp"
#include "core/macro.hpp"
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

namespace sylar
{
    static auto g_logger = SYLAR_LOG_NAME("system");

    /**
     * @brief IOManager 构造函数，初始化 I/O 管理器。
     *
     * @param threads 线程池的线程数量。指定调度器使用的线程数，通常与 CPU 核心数相关。
     * @param use_caller 是否将当前调用线程加入调度器线程池。如果为 true，则当前线程也会参与任务调度。
     * @param name 调度器名称。用于标识调度器实例。
     *
     * 初始化流程：
     * 1. 创建 epoll 实例，并设置管道以支持唤醒机制；
     * 2. 配置管道读端为非阻塞模式，并将其添加到 epoll 监听中；
     * 3. 初始化上下文容量并启动调度器。
     */
    IOManager::IOManager(size_t threads, bool use_caller, const std::string &name)
        : Scheduler(threads, use_caller, name)
    {
        // 创建 epoll 实例，用于监听文件描述符事件
        m_epfd = epoll_create(1);
        SYLAR_ASSERT(m_epfd > 0);

        // 创建管道，用于唤醒调度器
        int rt = pipe(m_tickleFds);
        SYLAR_ASSERT(!rt);

        epoll_event ev = {};
        ev.events = EPOLLIN | EPOLLET; // 监听读事件，使用边缘触发
        ev.data.fd = m_tickleFds[0];   // 设置监听的文件描述符为管道的读端

        // 将管道的读端设置为非阻塞模式，避免在读取时发生阻塞（若无数据可读或者是无法写入则立即返回）
        rt = fcntl(m_tickleFds[0], F_SETFL, O_NONBLOCK);
        SYLAR_ASSERT(!rt);

        // 将管道的读端添加到 epoll 实例中，以监听其事件
        rt = epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_tickleFds[0], &ev);
        SYLAR_ASSERT(!rt);

        // 初始化上下文存储容量，确保可以容纳足够的上下文对象
        contextResize(64);

        // 启动调度器，开始任务调度
        start();
    }

    /**
     * @brief 析构函数，用于清理 IOManager 实例所管理的资源。
     *
     * 该析构函数主要负责以下操作：
     * 1. 调用 stop() 方法停止事件循环；
     * 2. 关闭 epoll 文件描述符 m_epfd 和管道文件描述符 m_tickleFds；
     * 3. 遍历 m_fdContexts 容器，释放所有动态分配的 FdContext 对象。
     */
    IOManager::~IOManager()
    {
        stop();

        // 关闭 epoll 文件描述符和管道文件描述符
        close(m_epfd);
        close(m_tickleFds[0]);
        close(m_tickleFds[1]);

        // 遍历并释放所有的 FdContext 对象
        for (size_t i = 0; i < m_fdContexts.size(); ++i)
        {
            if (m_fdContexts[i])
            {
                delete m_fdContexts[i];
            }
        }
    }

    /**
     * @brief 向 I/O 管理器添加一个指定的事件。
     *
     * 该函数为指定文件描述符 `fd` 添加一个事件类型 `event`，并绑定回调函数 `cb`。
     * 如果文件描述符上下文数组不足，则会进行扩容操作。同时，该函数还确保不会重复添加相同类型的事件。
     * 最后，它通过 epoll_ctl 将事件注册到 epoll 实例中，并更新相关的上下文信息。
     *
     * @param fd 文件描述符，用于标识要监控的文件或套接字。
     * @param event 要添加的事件类型，来自 Event 枚举（如读、写等）。
     * @param cb 可选的回调函数，当事件触发时执行。如果未提供回调函数，则使用当前协程作为事件处理逻辑。
     * @return bool 返回 true 表示成功添加事件；返回 false 表示添加失败。
     */
    bool IOManager::addEvent(int fd, Event event, std::function<void()> cb)
    {
        FdContext *fd_ctx = nullptr;

        // ====================取出对应fd上下文====================
        // 加读锁访问 m_fdContexts，检查是否已存在对应 fd 的上下文
        RWMutexType::ReadLock lock(m_mutex);
        if ((int)m_fdContexts.size() > fd) // 空间够
        {
            fd_ctx = m_fdContexts[fd];
            lock.unlock(); // 防止和第二个lock2冲突
        }
        else
        {
            lock.unlock(); // 防止和第一个lock2冲突

            // 若上下文数组不足，加写锁进行扩容
            RWMutexType::WriteLock lock2(m_mutex);
            contextResize(fd * 1.5);
            fd_ctx = m_fdContexts[fd];
        }

        // ====================将事件注册到epoll实例中====================
        // 锁定 fd 上下文以修改其事件状态
        FdContext::MutexType::Lock lock2(fd_ctx->mutex);
        if (fd_ctx->events & event)
        {
            // 确保不会重复添加相同的事件类型
            SYLAR_LOG_ERROR(g_logger) << "addEvent assert fd=" << fd
                                      << " event=" << event
                                      << " fd_ctx.event=" << fd_ctx->events;
            SYLAR_ASSERT(!(fd_ctx->events & event));
        }

        // 根据已有事件决定是新增还是修改 epoll 监控
        int op = fd_ctx->events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
        epoll_event ev = {};
        ev.events = fd_ctx->events | event | EPOLLET; // 设置边缘触发模式
        ev.data.ptr = fd_ctx;

        // 使用 epoll_ctl 注册事件
        int rt = epoll_ctl(m_epfd, op, fd, &ev);
        if (rt)
        {
            // 记录错误日志并返回失败
            SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", " << op << ", " << fd << ", " << ev.events << "): "
                                      << rt << " (" << errno << ") (" << strerror(errno) << ")";
            return -1;
        }

        // ====================更新上下文信息====================
        // 更新事件计数及上下文中的事件掩码
        ++m_pendingEventCount;
        fd_ctx->events = (Event)(fd_ctx->events | event);

        // 初始化事件上下文
        FdContext::EventContext &event_ctx = fd_ctx->getContext(event);

        if (event_ctx.coroutine)
        {
            // SYLAR_LOG_WARN(g_logger) << "Event context already has coroutine or callback, resetting it. fd="
            //                          << fd << " event=" << event;
            fd_ctx->resetContext(event_ctx);
        }
        SYLAR_ASSERT(!event_ctx.scheduler);
        SYLAR_ASSERT(!event_ctx.coroutine);
        SYLAR_ASSERT(!event_ctx.cb);

        event_ctx.scheduler = Scheduler::GetThis();
        if (cb)
        {
            // 如果提供了回调函数，则绑定到事件上下文中
            event_ctx.cb.swap(cb);
        }
        else
        {
            // 否则使用当前协程作为事件处理逻辑
            event_ctx.coroutine = Coroutine::GetThis();
            SYLAR_ASSERT(event_ctx.coroutine->getState() == Coroutine::EXEC);
        }

        return 0;
    }

    /**
     * @brief 删除指定文件描述符的事件监听
     *
     * 该函数用于从 I/O 管理器中删除指定文件描述符的特定事件监听。如果事件删除成功，
     * 则返回 true；否则返回 false。
     *
     * @param fd 文件描述符，表示需要操作的文件或套接字
     * @param event 要删除的事件类型 (Event)，例如读事件、写事件等
     * @return bool 返回是否成功删除事件监听：
     *              - true 表示事件删除成功
     *              - false 表示事件删除失败（如文件描述符无效或事件未注册）
     */
    bool IOManager::delEvent(int fd, Event event)
    {
        // ====================获取文件描述符对应的 FdContext====================
        // 加读锁以保护 m_fdContexts 的并发访问
        RWMutexType::ReadLock lock(m_mutex);
        if ((int)m_fdContexts.size() <= fd)
        {
            return false; // 文件描述符超出范围，直接返回失败
        }

        FdContext *fd_ctx = m_fdContexts[fd];
        lock.unlock();

        // ====================判断删除的事件是否存在====================
        // 对 fd_ctx 加锁，确保对其事件的修改是线程安全的
        FdContext::MutexType::Lock lock2(fd_ctx->mutex);
        if (!(fd_ctx->events & event))
        {
            return false; // 如果目标事件不存在，直接返回失败
        }

        // ====================从epoll中删除事件监听====================
        // 计算新的事件集合，并决定 epoll 操作类型
        Event new_events = (Event)(fd_ctx->events & ~event); // 移除指定事件
        int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL; // 根据剩余事件决定修改还是删除
        epoll_event ev = {};
        ev.events = new_events | EPOLLET; // 设置边缘触发模式
        ev.data.ptr = fd_ctx;

        // 调用 epoll_ctl 更新事件监听
        int rt = epoll_ctl(m_epfd, op, fd, &ev);
        if (rt)
        {
            // 如果 epoll_ctl 失败，记录错误日志并返回失败
            SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", " << op << ", " << fd << ", " << ev.events << "): "
                                      << rt << " (" << errno << ") (" << strerror(errno) << ")";
            return false;
        }

        // ====================更新上下文信息====================
        --m_pendingEventCount;       // 减少待处理事件计数
        fd_ctx->events = new_events; // 更新文件描述符上下文中的事件集合

        // 重置与目标事件相关的上下文
        FdContext::EventContext &event_ctx = fd_ctx->getContext(event);
        fd_ctx->resetContext(event_ctx);
        return true; // 成功删除事件
    }

    /**
     * @brief 取消指定文件描述符上的事件监听。
     *
     * 该函数通过epoll机制取消对指定文件描述符(fd)上特定事件(event)的监听。
     * 如果对应的文件描述符不存在，或者没有对应的事件被监听，则返回false表示操作失败。
     * 否则，会根据新事件集合更新epoll的监听行为(修改或删除)，并在成功后减少待处理事件计数。
     *
     * @param fd 文件描述符，表示需要取消监听的文件。
     * @param event 要取消监听的事件类型。
     * @return true 如果事件取消成功。
     * @return false 如果事件取消失败（例如fd无效，或事件未注册）。
     */
    bool IOManager::cancelEvent(int fd, Event event)
    {
        // 加读锁以保护m_fdContexts容器的并发访问
        RWMutexType::ReadLock lock(m_mutex);
        if ((int)m_fdContexts.size() <= fd)
        {
            return false; // 文件描述符超出范围，直接返回false
        }

        FdContext *fd_ctx = m_fdContexts[fd];
        lock.unlock();

        // 对文件描述符上下文加锁，确保对其事件掩码的安全访问
        FdContext::MutexType::Lock lock2(fd_ctx->mutex);
        if (!(fd_ctx->events & event))
        {
            return false; // 如果当前文件描述符未监听该事件，直接返回false
        }

        // 计算新的事件集合，并确定epoll操作类型
        Event new_events = (Event)(fd_ctx->events & ~event);
        int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
        epoll_event ev = {};
        ev.events = new_events | EPOLLET;
        ev.data.ptr = fd_ctx;

        // 使用epoll_ctl修改或删除事件监听
        int rt = epoll_ctl(m_epfd, op, fd, &ev);
        if (rt)
        {
            SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", " << op << ", " << fd << ", " << ev.events << "): "
                                      << rt << " (" << errno << ") (" << strerror(errno) << ")";
            return false; // epoll_ctl调用失败时记录错误日志并返回false
        }

        // 触发相关事件的回调函数，并减少待处理事件计数
        fd_ctx->triggerEvent(event);
        --m_pendingEventCount;

        return true; // 返回true表示事件已成功取消
    }

    /**
     * @brief 取消指定文件描述符上的所有事件。
     *
     * 该函数会取消指定文件描述符 fd 上注册的所有事件，并清理相关的上下文信息。
     * 如果文件描述符上没有注册任何事件，或者文件描述符超出范围，则直接返回 false。
     * 否则，它会从 epoll 实例中删除该文件描述符的事件监听，并触发相应的回调函数（如果有）。
     *
     * @param fd 文件描述符，表示需要取消事件的目标。
     * @return bool 返回是否成功取消了 fd 上的所有事件。如果成功取消，返回 true；否则返回 false。
     */
    bool IOManager::cancelAll(int fd)
    {
        RWMutexType::ReadLock lock(m_mutex);
        // 检查 fd 是否在合法范围内
        if ((int)m_fdContexts.size() <= fd)
        {
            return false;
        }

        FdContext *fd_ctx = m_fdContexts[fd];
        lock.unlock();

        FdContext::MutexType::Lock lock2(fd_ctx->mutex);
        // 如果 fd 上未注册任何事件，直接返回 false
        if (!(fd_ctx->events))
        {
            return false;
        }

        // 从 epoll 实例中删除该文件描述符的事件监听
        int op = EPOLL_CTL_DEL;
        epoll_event ev = {};
        ev.events = 0;
        ev.data.ptr = fd_ctx;

        int rt = epoll_ctl(m_epfd, op, fd, &ev);
        if (rt)
        {
            SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", " << op << ", " << fd << ", " << ev.events << "): "
                                      << rt << " (" << errno << ") (" << strerror(errno) << ")";
            return false;
        }

        // 触发 READ 事件的回调函数（如果已注册）
        if (fd_ctx->events & Event::READ)
        {
            fd_ctx->triggerEvent(READ);
            --m_pendingEventCount;
        }

        // 触发 WRITE 事件的回调函数（如果已注册）
        if (fd_ctx->events & Event::WRITE)
        {
            fd_ctx->triggerEvent(WRITE);
            --m_pendingEventCount;
        }

        // 确保所有事件都已被正确清理
        SYLAR_ASSERT(fd_ctx->events == 0)
        return true;
    }

    IOManager *IOManager::GetThis()
    {
        return dynamic_cast<IOManager *>(Scheduler::GetThis());
    }

    /**
     * @brief 触发 IOManager 的调度器以唤醒一个空闲线程。
     *
     * 此函数用于通知 IOManager 中的一个空闲线程进行任务调度。
     * 如果当前没有空闲线程，则直接返回，不执行任何操作。
     *
     * 主要逻辑：
     * - 首先检查是否存在空闲线程。如果不存在空闲线程，函数立即返回。
     * - 若存在空闲线程，通过管道写入一个字节 "T" 来触发调度器。
     *   - 这里的 m_tickleFds 是一个管道文件描述符对，其中 [1] 是写端。
     *   - 写入成功后，使用断言确保写入的字节数为 1。
     *
     * @note 该函数无参数，也无返回值。
     */
    void IOManager::tickle()
    {
        // SYLAR_LOG_DEBUG(g_logger) << "tickle begin";
        //  检查是否有空闲线程，若无则直接返回
        if (!hasIdleThreads())
        {
            return;
        }
        // 通过管道写入一个字节 "T" 以触发调度器
        int rt = write(m_tickleFds[1], "T", 1);
        SYLAR_ASSERT(rt == 1)
        // SYLAR_LOG_DEBUG(g_logger) << "tickle end";
    }

    bool IOManager::stopping(uint64_t &timeout)
    {
        timeout = getNextTimer();
        return timeout == ~0ull && m_pendingEventCount == 0 && Scheduler::stopping();
    }

    bool IOManager::stopping()
    {
        uint64_t timeout;
        return stopping(timeout);
    }

    /**
     * @brief 空闲循环，用于管理I/O事件。
     *
     * 该函数使用epoll_wait持续监控文件描述符的I/O事件。
     * 它处理停止条件、处理唤醒事件（tickle event）并为受监控的文件描述符触发适当的读写回调。
     *
     * 函数会根据活动的I/O事件动态调整epoll事件设置，并高效地管理待处理事件计数。
     *
     * 参数：
     * - 无显式参数（使用IOManager的成员变量）
     *
     * 返回值：
     * - void: 在正常情况下不会返回；无限循环运行直到被停止。
     */
    void IOManager::idle()
    {
        SYLAR_LOG_DEBUG(g_logger) << "idle";
        // 分配 epoll_event 数组并使用智能指针管理内存，用于存储 epoll 等待到的事件
        epoll_event *events = new epoll_event[64]();
        std::shared_ptr<epoll_event> shared_event(events, [](epoll_event *ptr)
                                                  { delete[] ptr; });

        // 主空闲循环，持续运行直到满足停止条件
        while (true)
        {
            uint64_t next_timeout = 0;
            if (stopping(next_timeout)) // 停止监听
            {
                SYLAR_LOG_INFO(g_logger) << "name=" << getName() << " idle stopping exit";
                break;
            }

            int rt = 0;
            // 使用最大超时时间等待epoll事件
            do
            {
                static const int MAX_TIMEOUT = 3000;
                if (next_timeout != ~0ull)
                {
                    next_timeout = (int)next_timeout > MAX_TIMEOUT ? MAX_TIMEOUT : next_timeout;
                }
                else
                {
                    next_timeout = MAX_TIMEOUT;
                }
                rt = epoll_wait(m_epfd, events, 64, (int)next_timeout);
                if (rt < 0 && errno == EINTR)
                {
                    // SYLAR_LOG_DEBUG(g_logger) << "epoll_wait error";
                }
                else
                {
                    // SYLAR_LOG_DEBUG(g_logger) << "epoll wait rt=" << rt;
                    break;
                }
            } while (true);

            std::vector<std::function<void()>> cbs;
            listExpiredCb(cbs);
            if (!cbs.empty())
            {
                // SYLAR_LOG_DEBUG(g_logger) << "on timer cbs.size=" << cbs.size();
                schedule(cbs.begin(), cbs.end());
                cbs.clear();
            }

            // 处理由epoll_wait触发的事件
            for (int i = 0; i < rt; ++i)
            {
                epoll_event &event = events[i];

                // 如果事件来自用于唤醒调度器的管道，读取并清空管道中的数据
                if (event.data.fd == m_tickleFds[0])
                {
                    uint8_t dummy;
                    while (read(m_tickleFds[0], &dummy, 1) == 1)
                        ;
                    continue;
                }

                // 获取与文件描述符关联的上下文对象
                FdContext *fd_ctx = (FdContext *)event.data.ptr;
                FdContext::MutexType::Lock lock(fd_ctx->mutex);

                /*如果发生错误或挂起，启用读和写的监控，
                保证即使在错误或挂起的情况下，
                仍然能够处理 fd 上可能存在的未读或未写数据
                */
                if (event.events & (EPOLLERR | EPOLLHUP))
                {
                    event.events |= (EPOLLIN | EPOLLOUT) & fd_ctx->events;
                }

                int real_events = NONE;

                // 确定实际发生的事件
                if (event.events & EPOLLIN)
                {
                    real_events |= READ;
                }
                if (event.events & EPOLLOUT)
                {
                    real_events |= WRITE;
                }

                // 如果没有设置相关事件则跳过触发事件
                if ((fd_ctx->events & real_events) == NONE)
                {
                    continue;
                }

                // 调整剩余事件并相应更新epoll设置
                int left_events = (fd_ctx->events & ~real_events);
                int op = left_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
                event.events = left_events | EPOLLET;

                int rt2 = epoll_ctl(m_epfd, op, fd_ctx->fd, &event);
                if (rt2)
                {
                    SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", " << op << ", "
                                              << fd_ctx->fd << ", " << event.events << "): "
                                              << rt2 << " (" << errno << ") (" << strerror(errno) << ")";
                    continue;
                }

                // 如果适用，则触发读事件回调
                if (real_events & READ)
                {
                    fd_ctx->triggerEvent(READ);
                    --m_pendingEventCount;
                }

                // 如果适用，则触发写事件回调
                if (real_events & WRITE)
                {
                    fd_ctx->triggerEvent(WRITE);
                    --m_pendingEventCount;
                }
            }

            // 将控制权交回协程调度器
            Coroutine::ptr cur = Coroutine::GetThis();
            auto raw_ptr = cur.get();
            cur.reset();

            raw_ptr->swapOut();
        }
    }

    void IOManager::onTimerInsertedAtFront()
    {
        tickle();
    }

    /**
     * @brief 调整文件描述符上下文数组的大小。
     *
     * 此函数将 m_fdContexts 数组调整为指定的大小。对于新添加的每个元素，
     * 如果对应的上下文为空，将分配新的 FdContext 对象并初始化其文件描述符。
     *
     * @param size 要调整的目标大小。
     */
    void IOManager::contextResize(size_t size)
    {
        m_fdContexts.resize(size);

        // 遍历数组，为未初始化的元素分配和设置文件描述符上下文
        for (size_t i = 0; i < m_fdContexts.size(); ++i)
        {
            if (!m_fdContexts[i]) // 如果当前索引的元素为空
            {
                m_fdContexts[i] = new FdContext;
                m_fdContexts[i]->fd = i; // 设置文件描述符
            }
        }
    }

    /**
     * @brief 获取指定事件类型的上下文对象。
     *
     * 根据传入的事件类型，返回对应的事件上下文（EventContext）对象。
     *
     * @param event 事件类型，取值为 Event 枚举值，包括 READ 和 WRITE。
     *              - Event::READ: 表示读事件。
     *              - Event::WRITE: 表示写事件。
     * @return IOManager::FdContext::EventContext&
     *         返回与事件类型对应的 EventContext 引用：
     *         - 如果 event 是 Event::READ，则返回 read 上下文。
     *         - 如果 event 是 Event::WRITE，则返回 write 上下文。
     *         - 如果 event 是其他值，将触发断言错误。
     */
    IOManager::FdContext::EventContext &IOManager::FdContext::getContext(Event event)
    {
        switch (event)
        {
        case Event::READ:
            // 返回读事件上下文
            return read;
        case Event::WRITE:
            // 返回写事件上下文
            return write;
        default:
            SYLAR_ASSERT2(false, "getContext");
        }
    }

    /**
     * @brief 重置指定的 EventContext 对象的状态。
     *
     * 该函数会将传入的 EventContext 对象的所有成员变量重置为初始状态，
     * 包括清空调度器指针、重置协程上下文以及清空回调函数。
     *
     * @param event 引用类型的 EventContext 对象，表示需要被重置的事件上下文。
     *              - scheduler: 指向调度器的指针，会被设置为 nullptr。
     *              - coroutine: 协程上下文的智能指针，会调用其 reset 方法进行重置。
     *              - cb: 回调函数指针，会被设置为 nullptr。
     */
    void IOManager::FdContext::resetContext(EventContext &event)
    {
        // 将调度器指针重置为 nullptr，表示该事件不再绑定到任何调度器。
        event.scheduler = nullptr;

        // 重置协程上下文，释放与其关联的资源。
        event.coroutine.reset();

        // 清空回调函数指针，确保不会保留任何旧的回调逻辑。
        event.cb = nullptr;
    }

    /**
     * @brief 触发指定的事件类型，并根据事件上下文调度相应的回调或协程。
     *
     * 此函数会触发指定的事件，清除已经处理的事件标志位，
     * 并根据事件上下文中存储的回调函数或协程进行调度。
     *
     * @param event 要触发的事件类型（读、写或其他）。
     *              必须是当前 FdContext 已注册的事件之一。
     *
     * 详细流程：
     * 1. 验证传入的事件是否为当前已注册的事件；
     * 2. 清除已触发事件的标志位；
     * 3. 获取事件上下文，并根据上下文内容调度回调函数或协程；
     * 4. 清空事件上下文中的调度器指针。
     */
    void IOManager::FdContext::triggerEvent(Event event)
    {
        // 确保传入的事件是当前已注册的事件之一
        SYLAR_ASSERT(events & event);

        // 清除已触发事件的标志位
        events = (Event)(events & ~event);

        // 获取与事件关联的上下文
        EventContext &event_ctx = getContext(event);

        // 根据上下文内容调度回调函数或协程
        if (event_ctx.cb)
        {
            // 如果存在回调函数，则调度该回调
            event_ctx.scheduler->schedule(event_ctx.cb);
        }
        else
        {
            // 如果不存在回调函数，则调度协程
            event_ctx.scheduler->schedule(event_ctx.coroutine);
        }

        // 清空事件上下文中的调度器指针
        event_ctx.scheduler = nullptr;

        return;
    }
}
