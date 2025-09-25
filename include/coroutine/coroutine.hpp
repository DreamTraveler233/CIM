#pragma once

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
            INIT,  // 初始化状态 - 协程刚创建或重置
            HOLD,  // 暂停状态 - 协程被挂起
            EXEC,  // 执行状态 - 协程正在执行
            TERM,  // 终止状态 - 协程执行完毕
            READY, // 就绪状态 - 协程准备执行
            EXCEPT // 异常状态 - 协程执行出错
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

    class MallocStackAllocator
    {
    public:
        static void *Alloc(size_t size);
        static void Dealloc(void *ptr, size_t size);
    };
}