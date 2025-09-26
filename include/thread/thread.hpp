#pragma once

#include "noncopyable.hpp"
#include "semaphore.hpp"
#include <thread>
#include <string>
#include <functional>
#include <memory>

namespace sylar
{
    class Thread : public Noncopyable
    {
    public:
        using ptr = std::shared_ptr<Thread>;

        Thread(std::function<void()> cb, const std::string &name);
        ~Thread();

        pid_t getId() const;
        const std::string &getName() const;

        void join();

        static Thread *GetThis();
        static const std::string &GetName();
        static void SetName(const std::string &name);

    private:
        static void *run(void *arg);

    private:
        pid_t m_id = -1;            // 线程ID
        pthread_t m_thread = 0;     // 线程句柄
        std::function<void()> m_cb; // 线程回调函数
        std::string m_name;         // 线程名称
        Semaphore m_semaphore;      // 信号量
    };
}