#include "coroutine.hpp"
#include "config/config.hpp"
#include "core/macro.hpp"
#include "runtime/scheduler.hpp"
#include "core/util.hpp"
#include <atomic>

namespace sylar
{
    static auto g_logger = SYLAR_LOG_NAME("system");

    static std::atomic<uint64_t> s_coroutine_id = {0};    // 协程id
    static std::atomic<uint64_t> s_coroutine_count = {0}; // 全局协程计数器

    // 局部线程变量
    static thread_local Coroutine *t_coroutine = nullptr;            // 当前正在运行的协程
    static thread_local Coroutine::ptr t_thread_coroutine = nullptr; // 当前线程的主协程

    // 定义配置项--协程的栈大小--1MB
    static ConfigVar<uint32_t>::ptr g_coroutine_stack_size =
        Config::Lookup<uint32_t>("coroutine.stack_size", 1024 * 1024, "coroutine stack size");

    using StackAllocator = MallocStackAllocator;

    /**
     * @brief 构造一个主协程对象
     * @details 初始化协程状态为执行状态，设置当前协程实例，并获取当前上下文环境
     *          同时增加全局协程计数器
     * 1、主协程代表当前线程的执行环境，因此当主协程创建时便是处于执行状态
     * 2、将当前协程设置为局部线程变量的值，表示该协程运行在该线程上
     * 3、将当前上下文状态（线程状态）保存到 m_ctx 中，用于恢复协程执行
     */
    Coroutine::Coroutine()
        : m_state(State::EXEC)
    {
        if (getcontext(&m_ctx))
        {
            SYLAR_ASSERT2(false, "getcontext");
            return; // 如果有适当的错误处理机制，应该在这里处理
        }

        SetThis(this);
        ++s_coroutine_count;
        // SYLAR_LOG_DEBUG(g_logger) << "Coroutine::Coroutine() id=" << m_id;
    }

    /**
     * @brief 构造一个协程对象
     * @param cb 协程要执行的回调函数
     * @param stack_size 协程栈大小，如果为0则使用配置的默认值
     * @details 初始化协程ID、状态和执行环境，为协程分配执行栈空间
     */
    Coroutine::Coroutine(std::function<void()> cb, size_t stack_size, bool use_caller)
        : m_id(++s_coroutine_id),
          m_cb(cb)

    {
        ++s_coroutine_count;

        // 使用局部变量管理栈空间，确保异常安全性
        void *stack = nullptr;
        size_t stack_size_temp = stack_size ? stack_size : g_coroutine_stack_size->getValue();

        try
        {
            // 设置协程的栈空间
            stack = StackAllocator::Alloc(stack_size_temp);
            m_stack_size = stack_size_temp;
            m_stack = stack;
            m_ctx.uc_link = nullptr; // 指定协程执行完成后，返回的栈环境
            m_ctx.uc_stack.ss_sp = m_stack;
            m_ctx.uc_stack.ss_size = m_stack_size;

            // 获取当前线程上下文环境
            if (getcontext(&m_ctx))
            {
                SYLAR_ASSERT2(false, "getcontext");
            }

            if (use_caller)
            { // 设置协程的入口函数
                makecontext(&m_ctx, &CallerMainFunc, 0);
            }
            else
            {
                makecontext(&m_ctx, &MainFunc, 0);
            }
        }
        catch (...)
        {
            // 如果发生异常，释放已分配的栈空间
            if (stack)
            {
                StackAllocator::Dealloc(stack, stack_size_temp);
            }
            --s_coroutine_count;
            throw; // 重新抛出异常
        }
        // SYLAR_LOG_DEBUG(g_logger) << "Coroutine::Coroutine()" << " id=" << m_id;
    }

    /**
     * @brief 协程析构函数，负责清理协程资源
     *
     * 析构协程对象时，根据协程类型进行不同的清理操作：
     * 1. 对于子协程，释放其栈空间
     * 2. 对于主协程，处理当前执行上下文的相关状态
     * 同时维护全局协程计数器
     */
    Coroutine::~Coroutine()
    {
        // SYLAR_LOG_DEBUG(g_logger) << "Coroutine::~Coroutine" << " id=" << m_id;
        if (m_stack) // 说明为子协程
        {
            SYLAR_ASSERT(m_state == State::TERM ||
                         m_state == State::INIT ||
                         m_state == State::EXCEPT);
            StackAllocator::Dealloc(m_stack, m_stack_size);
        }
        else // 说明为主协程
        {
            SYLAR_ASSERT(!m_cb);
            SYLAR_ASSERT(m_state == State::EXEC);

            Coroutine *cur = t_coroutine;
            if (cur == this)
            {
                SetThis(nullptr);
            }
        }
        --s_coroutine_count;
    }

    /**
     * @brief 重置协程
     * @param cb 协程的回调函数
     *
     * 1、验证协程状态是否为终止、初始化或异常状态之一
     * 2、更新协程的回调函数
     * 3、重新初始化协程上下文环境，包括栈空间和执行函数
     * 4、将协程状态设置为初始化状态
     */
    void Coroutine::reset(std::function<void()> cb)
    {
        SYLAR_ASSERT(m_stack);
        SYLAR_ASSERT(m_stack_size > 0);
        SYLAR_ASSERT(m_state == State::TERM ||
                     m_state == State::INIT ||
                     m_state == State::EXCEPT);
        m_cb = cb;
        if (getcontext(&m_ctx)) // 获取当前上下文环境
        {
            SYLAR_ASSERT2(false, "getcontext");
        }
        m_ctx.uc_link = nullptr;
        m_ctx.uc_stack.ss_sp = m_stack;
        m_ctx.uc_stack.ss_size = m_stack_size;

        makecontext(&m_ctx, &MainFunc, 0);
        m_state = State::INIT;
    }

    /**
     * @brief 切换到当前线程运行
     * @details 将当前协程设置为正在执行状态，并使用swapcontext函数进行上下文切换，
     *          从线程的主协程切换到当前协程
     */
    void Coroutine::swapIn()
    {
        // 把当前运行协程设置为该子协程
        SetThis(this);
        SYLAR_ASSERT(m_state != State::EXEC);
        m_state = State::EXEC;

        // 从主协程切换到当前线程（子协程）
        if (swapcontext(&Scheduler::GetMainCoroutine()->m_ctx, &m_ctx))
        {
            SYLAR_ASSERT2(false, "swapcontext");
        }
    }

    /**
     * @brief 从子协程切换回主协程
     * @details 将执行权从当前协程切换回主协程（线程协程），保存当前协程的上下文环境，
     *          并恢复主协程的上下文环境继续执行
     */
    void Coroutine::swapOut()
    {
        // 从当前线程（子协程）切换回主协程
        SetThis(Scheduler::GetMainCoroutine());
        if (swapcontext(&m_ctx, &Scheduler::GetMainCoroutine()->m_ctx))
        {
            SYLAR_ASSERT2(false, "swapcontext");
        }
    }

    void Coroutine::call()
    {
        SetThis(this);
        // SYLAR_ASSERT(m_state != State::EXEC &&
        //              m_state != State::TERM &&
        //              m_state != State::EXCEPT);
        m_state = State::EXEC;
        // SYLAR_LOG_INFO(g_logger) << getId();
        if (swapcontext(&t_thread_coroutine->m_ctx, &m_ctx))
        {
            SYLAR_ASSERT2(false, "swapcontext");
        }
    }

    void Coroutine::back()
    {
        SetThis(t_thread_coroutine.get());
        if (swapcontext(&m_ctx, &t_thread_coroutine->m_ctx))
        {
            SYLAR_ASSERT2(false, "swapcontext");
        }
    }

    uint64_t Coroutine::getId() const
    {
        return m_id;
    }

    Coroutine::State Coroutine::getState() const
    {
        return m_state;
    }

    void Coroutine::setState(State state)
    {
        m_state = state;
    }

    /**
     * @brief 设置当前协程
     * @param val 指向协程对象的指针
     * @details 将线程局部变量 t_coroutine 设置为指定的协程对象，
     *          用于标识当前正在运行的协程
     */
    void Coroutine::SetThis(Coroutine *val)
    {
        t_coroutine = val;
    }

    /**
     * @brief 获取当前协程
     * @return 当前协程的智能指针
     * @details 返回当前正在执行的协程实例。如果当前没有设置协程，
     *          则创建一个新的主协程作为当前线程的默认协程
     *
     * 这里获取当前协程，要么是主协程，要么是子协程，如果当前协程为空，
     * 说明当前线程还没有主协程，则创建一个主协程作为当前线程的默认协程
     */
    Coroutine::ptr Coroutine::GetThis()
    {
        if (t_coroutine)
        {
            return t_coroutine->shared_from_this();
        }
        Coroutine::ptr main_coroutine(new Coroutine);
        SYLAR_ASSERT(t_coroutine == main_coroutine.get());
        t_thread_coroutine = main_coroutine;
        return t_coroutine->shared_from_this();
    }

    void Coroutine::YieldToReady()
    {
        Coroutine::ptr cur = GetThis();
        cur->m_state = State::READY;
        cur->swapOut();
    }

    void Coroutine::YieldToHold()
    {
        Coroutine::ptr cur = GetThis();
        cur->m_state = State::HOLD;
        cur->swapOut();
    }

    /**
     * @brief 获取协程总数
     * @return 返回全局协程计数器的值
     * @details 返回当前创建的协程总数，用于统计和监控
     */
    uint64_t Coroutine::TotalCoroutines()
    {
        return s_coroutine_count;
    }

    /**
     * @brief 协程的主执行函数
     * @details 执行当前协程的回调函数，并处理可能发生的异常。
     *          执行完成后将协程状态设置为TERM（终止）
     */
    void Coroutine::MainFunc()
    {
        // 获取当前正在运行的协程
        Coroutine::ptr cur = GetThis();
        SYLAR_ASSERT(cur);

        try
        {
            cur->m_cb(); // 执行协程的回调函数
            cur->m_cb = nullptr;
            cur->m_state = State::TERM;
        }
        catch (std::exception &ex)
        {
            cur->m_state = State::EXCEPT;
            SYLAR_LOG_ERROR(g_logger) << "coroutine exception: " << ex.what()
                                      << " coroutine id: " << cur->getId()
                                      << std::endl
                                      << BacktraceToString();
        }
        catch (...)
        {
            cur->m_state = State::EXCEPT;
            SYLAR_LOG_ERROR(g_logger) << "Coroutine exception";
        }

        // 协程执行完毕后，需要将控制权交还给主协程
        auto p = cur.get();
        cur.reset();
        p->swapOut();

        SYLAR_ASSERT2(false, "never reach coroutine id=" + std::to_string(p->getId()));
    }

    void Coroutine::CallerMainFunc()
    {
        // 获取当前正在运行的协程
        Coroutine::ptr cur = GetThis();
        SYLAR_ASSERT(cur);

        try
        {
            cur->m_cb(); // 执行协程的回调函数
            cur->m_cb = nullptr;
            cur->m_state = State::TERM;
        }
        catch (std::exception &ex)
        {
            cur->m_state = State::EXCEPT;
            SYLAR_LOG_ERROR(g_logger) << "coroutine exception: " << ex.what()
                                      << " coroutine id: " << cur->getId()
                                      << std::endl
                                      << BacktraceToString();
        }
        catch (...)
        {
            cur->m_state = State::EXCEPT;
            SYLAR_LOG_ERROR(g_logger) << "Coroutine exception";
        }

        // 协程执行完毕后，需要将控制权交还给主协程
        auto p = cur.get();
        cur.reset();
        p->back();

        SYLAR_ASSERT2(false, "never reach coroutine id=" + std::to_string(p->getId()));
    }

    /**
     * @brief 获取当前协程ID
     * @return 返回当前正在执行的协程ID，如果没有正在执行的协程则返回0
     * @details 通过线程局部变量t_coroutine获取当前协程，如果存在则返回其ID，否则返回0
     */
    uint64_t Coroutine::GetCoroutineId()
    {
        return t_coroutine ? t_coroutine->m_id : 0;
    }

    /**
     * @brief 分配协程栈空间
     * @param size 需要分配的栈空间大小
     * @return 指向分配的栈空间的指针
     */
    void *MallocStackAllocator::Alloc(size_t size)
    {
        return malloc(size);
    }

    /**
     * @brief 释放协程栈空间
     * @param ptr 指向需要释放的栈空间的指针
     * @param size 栈空间的大小
     */
    void MallocStackAllocator::Dealloc(void *ptr, size_t size)
    {
        free(ptr);
    }
}