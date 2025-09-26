#include "scheduler.hpp"
#include "macro.hpp"

namespace sylar
{
    static auto g_logger = SYLAR_LOG_NAME("system");

    // 线程局部变量，指向当前线程的调度器实例
    static thread_local Scheduler *t_scheduler = nullptr;
    // 线程局部变量，指向当前线程正在执行的协程
    static thread_local Coroutine *t_coroutine = nullptr;

    /**
     * @brief 构造一个Scheduler对象
     * @param[in] threads 线程数量
     * @param[in] use_caller 是否使用调用线程作为调度线程
     * @param[in] name 调度器名称
     */
    Scheduler::Scheduler(size_t threads, bool use_caller, const std::string &name)
        : m_name(name)
    {
        SYLAR_ASSERT(threads > 0);

        // 如果使用调用线程，则将当前线程作为调度线程之一
        if (use_caller)
        {
            Coroutine::GetThis(); // 初始化当前线程的主协程
            --threads;

            SYLAR_ASSERT(GetThis() == nullptr); // 当前线程的调度器实例为空
            t_scheduler = this;                 // 设置当前线程的调度器

            // 创建根协程并绑定到当前调度器的run方法
            m_rootCoroutine.reset(new Coroutine(std::bind(&Scheduler::run, this)));
            Thread::SetName(m_name);
            t_coroutine = m_rootCoroutine.get(); // 设置当前线程的协程为根协程

            m_rootThread = GetThreadId(); // 记录主线程ID
            m_threadIds.push_back(m_rootThread);
        }
        else
        {
            m_rootThread = -1;
        }
        m_threadCount = threads;
    }

    Scheduler::~Scheduler()
    {
        SYLAR_ASSERT(m_stopping);
        if (GetThis() == this)
        {
            t_scheduler = nullptr;
        }
    }

    const std::string &Scheduler::getName() const { return m_name; }

    Scheduler *Scheduler::GetThis() { return t_scheduler; }
    void Scheduler::setThis() { t_scheduler = this; }

    Coroutine::ptr Scheduler::getMainCoroutine() { return t_coroutine->GetThis(); }

    void Scheduler::start()
    {
        MutexType::Lock lock(m_mutex);
        if (!m_stopping)
        {
            return;
        }
        m_stopping = false;
        SYLAR_ASSERT(m_threads.empty());

        m_threads.reserve(m_threadCount); // 提前分配内存
        for (size_t i = 0; i < m_threadCount; ++i)
        {
            m_threads[i] = (std::make_shared<Thread>(Thread(std::bind(&Scheduler::run, this), m_name + "_" + std::to_string(i))));
            m_threadIds.push_back(m_threads[i]->getId());
        }
    }

    void Scheduler::stop()
    {
        m_autoStop = true;
        if (m_rootCoroutine &&
            m_threadCount == 0 &&
            (m_rootCoroutine->getState() == Coroutine::State::TERM ||
             m_rootCoroutine->getState() == Coroutine::State::EXCEPT))
        {
            SYLAR_LOG_INFO(g_logger) << this << "stoped";
            m_stopping = true;

            if (stopping())
            {
                return;
            }
        }

        bool exit_on_this_coroutine = false;
        if (m_rootThread != -1)
        {
            SYLAR_ASSERT(GetThis() == this);
        }
        else
        {
            SYLAR_ASSERT(GetThis() != this);
        }

        m_stopping = true;
        for (size_t i = 0; i < m_threadCount; ++i)
        {
            tickle();
        }

        if (m_rootCoroutine)
        {
            tickle();
        }

        if (stopping())
        {
            return;
        }
    }

    /**
     * @brief 调度器的核心执行函数，负责调度和执行协程任务
     *
     * 该函数是调度器在线程中运行的核心逻辑，主要功能包括：
     * 1. 设置当前线程的调度器实例
     * 2. 初始化协程环境
     * 3. 循环处理协程队列中的任务
     * 4. 执行协程或回调函数
     * 5. 管理协程状态转换
     */
    /*
     1. 主协程(线程)运行
             ↓
     2. 调度器run()开始执行
             ↓
     3. 从任务队列取出协程任务
             ↓
     4. 调用 ct.coroutine->swapIn()
             ↓
     5. 切换到任务协程执行
             ↓
     6. 任务协程执行完毕或主动yield
             ↓
     7. 切换回主协程(或根协程)
             ↓
     8. 继续调度器run()循环
     */
    void Scheduler::run()
    {
        setThis(); // 设置当前线程的调度器实例

        // 如果不是主线程，则将主协程设置为当前线程正在执行的协程
        if (GetThreadId() != m_rootThread)
        {
            t_coroutine = Coroutine::GetThis().get();
        }

        // 创建空闲协程，当没有任务可执行时运行
        Coroutine::ptr idle = std::make_shared<Coroutine>(Coroutine(std::bind(&Scheduler::idle, this)));
        // 用于执行回调函数的协程
        Coroutine::ptr cb_coroutine;

        // 存储从协程队列中取出的协程或回调任务
        CoroutineAndThread ct;

        /*
        1、任务在正确的线程上执行（线程亲和性）
        2、正在执行的任务不会被重复调度（状态检查）
        3、线程间的任务负载得到均衡（tickle机制）
        4、整个过程是线程安全的（互斥锁保护）
        */
        while (true)
        {
            // ==========任务获取阶段==========
            ct.reset();             // 清除上一次循环中保存的任务，确保当前循环处理的是新任务
            bool tickle_me = false; // 初始化标志位，用于标记是否需要通知其他线程
            {
                // 加锁访问协程队列
                MutexType::Lock lock(m_mutex);
                auto it = m_coroutines.begin();
                while (it != m_coroutines.end())
                {
                    SYLAR_ASSERT(it->coroutine || it->cb);

                    // 当前任务指定了执行线程，且该线程不是当前线程，则跳过该任务，并标记为需要通知其他线程
                    if (it->threadId != -1 && it->threadId != GetThreadId())
                    {
                        ++it;
                        tickle_me = true;
                        continue;
                    }

                    // 如果it中保存的是协程，并且正在执行中，则跳过
                    if (it->coroutine && it->coroutine->getState() == Coroutine::State::EXEC)
                    {
                        ++it;
                        continue;
                    }

                    // 取出协程任务
                    ct = *it;
                    m_coroutines.erase(it);
                }
            }

            // ==========跨线程通知==========
            // 如果有其他线程需要被唤醒，则触发tickle
            if (tickle_me)
            {
                tickle();
            }

            // ==========处理协程任务==========
            if (ct.coroutine &&
                ct.coroutine->getState() != Coroutine::State::TERM &&
                ct.coroutine->getState() != Coroutine::State::EXCEPT)
            {
                ++m_activeThreadCount;
                // 进入目标协程
                ct.coroutine->swapIn();
                // 离开目标协程
                --m_activeThreadCount;
                // 如果协程状态为READY，说明协程主动让出了执行权，但仍需要继续执行，重新加入调度队列
                if (ct.coroutine->getState() == Coroutine::State::READY)
                {
                    schedule(ct.coroutine);
                }
                // 如果协程未结束且无异常，设置为HOLD状态
                else if (ct.coroutine->getState() != Coroutine::State::TERM &&
                         ct.coroutine->getState() != Coroutine::State::EXCEPT)
                {
                    ct.coroutine->setState(Coroutine::State::HOLD);
                }
                // 如果协程是终止状态（TERM）或异常状态（EXCEPT），结束该协程的任务
            }

            // ==========处理回调函数任务==========
            else if (ct.cb) // 说明这是一个回调函数任务而不是协程任务
            {
                // 回调协程复用
                if (cb_coroutine)
                {
                    cb_coroutine->reset(ct.cb);
                }
                else
                {
                    cb_coroutine.reset(new Coroutine(ct.cb));
                }

                ++m_activeThreadCount;
                // 进入回调函数
                cb_coroutine->swapIn();
                // 离开回调函数
                --m_activeThreadCount;

                // 如果协程状态为READY，重新加入调度队列
                if (cb_coroutine->getState() == Coroutine::State::READY)
                {
                    schedule(cb_coroutine);
                    cb_coroutine.reset();
                }
                // 如果协程未结束且无异常
                else if (cb_coroutine->getState() == Coroutine::State::TERM ||
                         cb_coroutine->getState() == Coroutine::State::EXCEPT)
                {
                    cb_coroutine->reset(nullptr);
                }
                // 其他情况设置为HOLD状态并重置协程指针
                else
                {
                    cb_coroutine->setState(Coroutine::State::HOLD);
                    cb_coroutine.reset();
                }
            }
            else if (idle)
            {
                if (idle->getState() == Coroutine::State::TERM)
                {
                    break;
                }

                ++m_idleThreadCount;
                // 运行空闲协程
                idle->swapIn();
                --m_idleThreadCount;

                // 如果空闲协程未结束且无异常，设置为HOLD状态
                if (idle->getState() != Coroutine::State::EXCEPT ||
                    idle->getState() != Coroutine::State::TERM)
                {
                    idle->setState(Coroutine::State::HOLD);
                }
            }
        }
    }
}