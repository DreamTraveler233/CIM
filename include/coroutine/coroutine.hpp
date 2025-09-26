#pragma once

#include "noncopyable.hpp"
#include <ucontext.h>
#include <functional>
#include <memory>

namespace sylar
{
    class Coroutine : public std::enable_shared_from_this<Coroutine>
    {
    public:
        using ptr = std::shared_ptr<Coroutine>;

        enum State
        {
            INIT,  // 初始化状态 - 表示协程刚创建或者是重置后，等待调度
            HOLD,  // 暂停状态 - 表示协程被挂起，等待外部条件满足，继续执行
            EXEC,  // 执行状态 - 表示协程正在执行中，当前拥有线程执行权
            TERM,  // 终止状态 - 表示协程已执行完毕，生命周期结束
            READY, // 就绪状态 - 表示协程主动让出执行权，但希望继续执行
            EXCEPT // 异常状态 - 表示协程执行过程中发生异常，已终止
        };

    private:
        Coroutine();

    public:
        Coroutine(std::function<void()> cb, size_t stack_size = 0);
        ~Coroutine();
        void reset(std::function<void()> cb);
        void swapIn();
        void swapOut();
        uint64_t getId() const;
        State getState() const;
        void setState(State state);

    public:
        static void SetThis(Coroutine *f);
        static Coroutine::ptr GetThis();
        static void YieldToReady();
        static void YieldToHold();
        static uint64_t TotalCoroutines();
        static void MainFunc();
        static uint64_t GetCoroutineId();

    private:
        uint64_t m_id = 0;           // 协程id
        uint32_t m_stack_size = 0;   // 协程栈大小
        State m_state = State::INIT; // 协程当前状态
        ucontext_t m_ctx;            // 协程上下文，用于保存和切换上下文环境
        void *m_stack = nullptr;     // 协程栈空间
        std::function<void()> m_cb;  // 协程要执行的回调函数
    };

    class MallocStackAllocator : public Noncopyable
    {
    public:
        static void *Alloc(size_t size);
        static void Dealloc(void *ptr, size_t size);
    };
}