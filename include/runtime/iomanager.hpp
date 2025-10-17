#pragma once

#include "scheduler.hpp"
#include "timer.hpp"

namespace sylar
{
    class IOManager : public Scheduler, public TimerManager
    {
    public:
        using ptr = std::shared_ptr<IOManager>;
        using RWMutexType = RWMutex;

        /**
         * @brief IO事件
         */
        enum Event
        {
            /// 无事件
            NONE = 0x0,
            /// 读事件(EPOLLIN)
            READ = 0x1,
            /// 写事件(EPOLLOUT)
            WRITE = 0x4,
        };

    private:
        /*
         FdContext 结构体的作用就是管理文件描述符（fd）上的事件上下文，
         并在特定事件（如读或写）发生时，切换到对应的事件上下文，
         执行绑定的协程或回调函数。
         */
        struct FdContext
        {
            using MutexType = Mutex;
            // 表示具体事件的上下文
            struct EventContext
            {
                Scheduler *scheduler = nullptr; // 指定执行事件的调度器
                Coroutine::ptr coroutine;       // 绑定到事件的协程对象
                std::function<void()> cb;       // 事件触发时执行的回调函数
            };

            EventContext &getContext(Event event);
            void resetContext(EventContext &event);
            void triggerEvent(Event event);

            int fd = 0;          // 事件绑定的文件描述符
            EventContext read;   // 读事件上下文
            EventContext write;  // 写事件上下文
            Event events = NONE; // 当前监听的事件类型
            MutexType mutex;     // 互斥锁
        };

    public:
        IOManager(size_t threads = 1, bool use_caller = true, const std::string &name = "");
        ~IOManager();

        // 1 success, -1 error
        bool addEvent(int fd, Event event, std::function<void()> cb = nullptr);
        bool delEvent(int fd, Event event);
        bool cancelEvent(int fd, Event event);
        bool cancelAll(int fd);

        static IOManager *GetThis();

    protected:
        virtual void tickle() override;
        virtual bool stopping() override;
        virtual void idle() override;

        virtual void onTimerInsertedAtFront() override;

        void contextResize(size_t size);
        bool stopping(uint64_t &timeout);

    private:
        int m_epfd = 0;                                // Epoll 文件描述符，用于事件监听
        int m_tickleFds[2];                            // 管道文件描述符对，用于唤醒 epoll_wait
        std::atomic<size_t> m_pendingEventCount = {0}; // 当前待处理的事件数量，使用原子类型保证线程安全
        RWMutexType m_mutex;                           // 读写锁，用于保护对文件描述符上下文的并发访问
        std::vector<FdContext *> m_fdContexts;         // 文件描述符上下文数组，每个文件描述符对应一个上下文
    };
}