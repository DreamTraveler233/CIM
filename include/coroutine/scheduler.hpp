#pragma once

#include "lock.hpp"
#include "coroutine.hpp"
#include "thread.hpp"
#include "noncopyable.hpp"
#include <list>
#include <vector>

/*
==============================协程调度器模型==============================

线程层面:
+---------------------+     +---------------------+     +---------------------+
|     主线程          |     |     工作线程1        |     |     工作线程2        |
|    协程调度器       |     |     协程调度器       |     |     协程调度器       |
|   (调用线程)        |     |                     |     |                     |
+----------+---------+     +----------+----------+     +----------+----------+
           |                           |                           |
           |                           |                           |
协程层面:   |                           |                           |
           |                           |                           |
+----------v----------+     +----------v----------+     +----------v----------+
|   主协程 (main)     |     |   主协程 (main)     |     |   主协程 (main)     |
|                     |     |                     |     |                     |
| +-----------------+ |     | +-----------------+ |     | +-----------------+ |
| | 根协程 (run)    | |     | | 任务协程A       | |     | | 任务协程B       | |
| | (仅主线程有)    | |     | |                 | |     | |                 | |
| +-----------------+ |     | +-----------------+ |     | +-----------------+ |
|                     |     |                     |     |                     |
| +-----------------+ |     | +-----------------+ |     | +-----------------+ |
| | 空闲协程(idle)  | |     | | 空闲协程(idle)  | |     | | 空闲协程(idle)  | |
| +-----------------+ |     | +-----------------+ |     | +-----------------+ |
|                     |     |                     |     |                     |
| +-----------------+ |     | +-----------------+ |     | +-----------------+ |
| | 回调协程        | |     | | 回调协程        | |     | | 回调协程        | |
| | (cb_coroutine)  | |     | | (cb_coroutine)  | |     | | (cb_coroutine)  | |
| +-----------------+ |     | +-----------------+ |     | +-----------------+ |
+---------------------+     +---------------------+     +---------------------+

主协程：
    1、每个线程都需要有一个协程作为执行上下文的基础
    2、当协程切换时，需要有一个目标协程返回到线程的正常执行流
    3、主协程就是这个"返回目标"
 */

namespace sylar
{
    class Scheduler : public Noncopyable
    {
    public:
        using ptr = std::shared_ptr<Scheduler>;
        using MutexType = Mutex;

        Scheduler(size_t threads = 1, bool use_caller = true, const std::string &name = "");
        virtual ~Scheduler();

        const std::string &getName() const;

        static Scheduler *GetThis();
        static Coroutine *GetMainCoroutine();

        void start();
        void stop();

        /**
         * @brief 用于调度单个协程或回调函数
         *
         * @param cb 要调度的协程或回调函数
         * @param tid 指定执行该任务的线程ID，默认为-1表示任意线程都可以执行
         */
        template <class CoroutineOrcb>
        void schedule(CoroutineOrcb cb, uint64_t tid = -1)
        {
            bool need_tickle = false; // 用于标记是否需要唤醒工作线程
            {
                MutexType::Lock lock(m_mutex);
                need_tickle = scheduleNolock(cb, tid);
            }
            if (need_tickle)
            {
                tickle(); // 唤醒工作线程
            }
        }

        /**
         * @brief 用于批量调度协程或回调函数
         *
         * @param begin 任务列表的起始迭代器
         * @param end 任务列表的结束迭代器
         */
        template <class InputIterator>
        void schedule(InputIterator begin, InputIterator end)
        {
            bool need_tickle = false; // 用于标记是否需要唤醒工作线程
            {
                MutexType::Lock lock(m_mutex);
                while (begin != end)
                {
                    // 将每个任务通过scheduleNolock添加到调度队列
                    need_tickle = scheduleNolock(&*begin, -1) || need_tickle;
                    ++begin;
                }
            }
            if (need_tickle)
            {
                tickle(); // 唤醒其他线程
            }
        }

    protected:
        virtual void tickle();
        virtual bool stopping();
        virtual void idle();

        void run();
        void setThis();
        bool hasIdleThreads();

    private:
        /**
         * @brief 用于在不加锁的情况下将协程或回调函数添加到调度队列中
         *
         * @param cb 要调度的协程或回调函数
         * @param tid 指定执行该任务的线程ID，默认为-1表示任意线程都可以执行
         * @return true 需要唤醒工作线程
         * @return false 不需要唤醒工作线程
         */
        template <class CoroutineOrcb>
        bool scheduleNolock(CoroutineOrcb cb, uint64_t tid)
        {
            // 如果队列不为空，说明有其他任务正在等待处理，工作线程应该已经在运行或即将运行
            // 如果队列为空，工作线程可能处于空闲状态，需要主动唤醒以处理新任务
            bool need_tickle = m_coroutines.empty();
            CoroutineAndThread ct(cb, tid);
            if (ct.coroutine || ct.cb)
            {
                m_coroutines.push_back(ct);
            }
            return need_tickle;
        }

    private:
        struct CoroutineAndThread
        {
            Coroutine::ptr coroutine; // 协程智能指针，存储待执行的协程对象
            std::function<void()> cb; // 回调函数，存储待执行的普通函数对象
            pid_t threadId;           // 线程ID，指定该任务应在哪个线程上执行，-1表示任意线程

            CoroutineAndThread(Coroutine::ptr c, uint64_t tid)
                : coroutine(c), threadId(tid)
            {
            }
            CoroutineAndThread(Coroutine::ptr *c, uint64_t tid)
                : threadId(tid)
            {
                coroutine.swap(*c);
            }
            CoroutineAndThread(std::function<void()> f, uint64_t tid)
                : cb(f), threadId(tid)
            {
            }
            CoroutineAndThread(std::function<void()> *f, uint64_t tid)
                : threadId(tid)
            {
                cb.swap(*f);
            }
            CoroutineAndThread()
                : threadId(-1)
            {
            }
            void reset()
            {
                coroutine = nullptr;
                cb = nullptr;
                threadId = -1;
            }
        };

    private:
        MutexType m_mutex;                          // 互斥锁，保护协程队列和线程安全
        std::vector<Thread::ptr> m_threads;         // 线程池，存储所有工作线程
        std::list<CoroutineAndThread> m_coroutines; // 待执行的协程队列，存储待调度的协程和回调函数
        Coroutine::ptr m_rootCoroutine;             // 主协程，调度器的根协程，负责调度其他协程
        std::string m_name;                         // 协程调度器的名称

    protected:
        std::vector<pid_t> m_threadIds;                // 线程ID列表，存储工作线程的ID
        size_t m_threadCount = 0;                      // 工作线程数量
        std::atomic<size_t> m_activeThreadCount = {0}; // 活跃线程数量（正在执行协程的线程数）
        std::atomic<size_t> m_idleThreadCount = {0};   // 空闲线程数量（等待任务的线程数）
        bool m_stopping = true;                        // 调度器是否正在停止
        bool m_autoStop = false;                       // 是否自动停止（当没有任务时自动停止）
        pid_t m_rootThread = 0;                        // 主线程ID（使用调用线程时的线程ID）
    };
}