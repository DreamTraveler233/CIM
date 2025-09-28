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
            m_rootCoroutine.reset(new Coroutine(std::bind(&Scheduler::run, this), 0, true));
            Thread::SetName(m_name);
            t_coroutine = m_rootCoroutine.get();

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

    Coroutine *Scheduler::GetMainCoroutine() { return t_coroutine; }

    /**
     * @brief 启动协程调度器
     *
     * 初始化并启动协程调度器，创建指定数量的工作线程来执行协程任务。
     * 如果调度器已经在运行则直接返回。
     *
     * @note 该函数只能调用一次，重复调用无效
     */
    void Scheduler::start()
    {
        MutexType::Lock lock(m_mutex);

        // 如果调度器正在运行，则直接返回
        if (!m_stopping)
        {
            return;
        }
        m_stopping = false;

        SYLAR_ASSERT(m_threads.empty());
        m_threads.resize(m_threadCount); // 提前分配内存
        for (size_t i = 0; i < m_threadCount; ++i)
        {
            // 创建工作线程，绑定调度器的run方法作为线程执行函数
            m_threads[i].reset(new Thread(std::bind(&Scheduler::run, this), m_name + "_" + std::to_string(i)));
            m_threadIds.push_back(m_threads[i]->getId());
        }

        lock.unlock();
    }

    void Scheduler::stop()
    {
        m_autoStop = true; // 设置自动停止标志

        // =================== 快速停止检查===================
        /*
        如果调度器只使用调用线程（没有额外的工作线程），
        并且根协程已经完成或出现异常，
        那么可以立即停止调度器，
        而不需要执行完整的停止流程
        */
        if (m_threadCount == 0 &&
            m_rootCoroutine &&
            (m_rootCoroutine->getState() == Coroutine::State::TERM ||
             m_rootCoroutine->getState() == Coroutine::State::INIT))
        {
            SYLAR_LOG_INFO(g_logger) << this << " stoped";
            m_stopping = true;

            // 检查是否可以安全停止协程调度器
            if (stopping())
            {
                return;
            }
        }

        // bool exit_on_this_coroutine = false;

        if (m_rootThread != -1)
        {
            // 使用调用线程作为调度线程之一
            SYLAR_ASSERT(GetThis() == this);
        }
        else
        {
            // 不使用调用线程作为调度线程
            SYLAR_ASSERT(GetThis() != this);
        }

        // ===================触发停止流程===================
        m_stopping = true;

        // 通知其他工作线程关闭调度器
        for (size_t i = 0; i < m_threadCount; ++i)
        {
            SYLAR_LOG_INFO(g_logger) << "word thread tickle";
            tickle();
        }

        // 通知主线程的关闭调度器
        if (m_rootCoroutine)
        {
            SYLAR_LOG_INFO(g_logger) << "m_rootCoroutine tickle";
            tickle();
        }

        if (m_rootCoroutine)
        {
            // while (!stopping())
            // {
            //     if (m_rootCoroutine->getState() == Coroutine::State::TERM ||
            //         m_rootCoroutine->getState() == Coroutine::State::EXCEPT)
            //     {
            //         m_rootCoroutine.reset(new Coroutine(std::bind(&Scheduler::run, this), 0, true));
            //         SYLAR_LOG_INFO(g_logger) << "root coroutine is term, reset";
            //         t_coroutine = m_rootCoroutine.get();
            //     }
            //     // 进入根协程，进行协程调度
            //     m_rootCoroutine->call();
            // }

            if (!m_stopping)
            {
                m_rootCoroutine->call();
            }
        }

        // 等待所有线程执行完任务然后退出
        std::vector<Thread::ptr> thrs;
        {
            MutexType::Lock lock(m_mutex);
            thrs.swap(m_threads);
        }

        for (auto &i : thrs)
        {
            i->join();
        }
    }

    void Scheduler::tickle()
    {
        SYLAR_LOG_INFO(g_logger) << "tickle";
    }

    bool Scheduler::stopping()
    {
        MutexType::Lock lock(m_mutex);
        return m_autoStop && m_coroutines.empty() && m_stopping && m_activeThreadCount == 0;
    }

    void Scheduler::idle()
    {
        SYLAR_LOG_INFO(g_logger) << "idle";
        while (!stopping())
        {
            Coroutine::YieldToHold();
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
        // return;
        SYLAR_LOG_INFO(g_logger) << m_name << " run";
        setThis(); // 设置当前线程的调度器实例

        if (GetThreadId() != m_rootThread)
        {
            t_coroutine = Coroutine::GetThis().get();
        }

        // 创建空闲协程，当没有任务可执行时运行
        Coroutine::ptr idle_coroutine(new Coroutine(std::bind(&Scheduler::idle, this)));
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
            bool tickle_me = false; // 是否需要通知其他线程
            bool is_active = false; // 线程是否处于活动状态
            {
                // 加锁访问协程队列
                MutexType::Lock lock(m_mutex);
                auto it = m_coroutines.begin();
                while (it != m_coroutines.end())
                {

                    // 当前任务指定了执行线程，且该线程不是当前线程，则跳过该任务，并标记为需要通知其他线程
                    if (it->threadId != -1 && it->threadId != GetThreadId())
                    {
                        ++it;
                        tickle_me = true;
                        continue;
                    }

                    SYLAR_ASSERT(it->coroutine || it->cb);

                    // 如果it中保存的是协程，并且正在执行中，则跳过
                    if (it->coroutine && it->coroutine->getState() == Coroutine::State::EXEC)
                    {
                        ++it;
                        continue;
                    }

                    // 取出协程任务
                    ct = *it;
                    m_coroutines.erase(it);
                    ++m_activeThreadCount;
                    is_active = true;
                    break;
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
            else
            {
                if (is_active)
                {
                    --m_activeThreadCount;
                    continue;
                }
                if (idle_coroutine->getState() == Coroutine::State::TERM)
                {
                    SYLAR_LOG_INFO(g_logger) << "idle coroutine term";
                    break;
                }

                ++m_idleThreadCount;
                idle_coroutine->swapIn();
                --m_idleThreadCount;

                // 如果空闲协程未结束且无异常，设置为HOLD状态
                if (idle_coroutine->getState() != Coroutine::State::EXCEPT &&
                    idle_coroutine->getState() != Coroutine::State::TERM)
                {
                    idle_coroutine->setState(Coroutine::State::HOLD);
                }
            }
        }
    }
}