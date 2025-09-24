#include "thread.hpp"
#include "macro.hpp"

namespace sylar
{
    static thread_local Thread *t_thread = nullptr;           // 指向当前线程的Thread对象
    static thread_local std::string t_thread_name = "UNKNOWN"; // 当前线程的名称

    static auto g_logger = SYLAR_LOG_NAME("system");

    Thread::Thread(std::function<void()> cb, const std::string &name)
        : m_cb(cb),
          m_name(name)
    {
        if (name.empty())
        {
            m_name = "UNKNOWN";
        }
        /**
         * pthread_create API 详解：
         *      int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
         *              void *(*start_routine)(void *), void *arg);
         *      thread：输出参数，返回新创建线程的ID
         *      attr：线程属性，通常传NULL使用默认属性
         *      start_routine：线程入口函数指针，线程创建后执行的函数
         *      arg：传递给线程函数的参数
         *
         *      成功返回0
         *      失败返回错误码
         */
        /**
         * 这里将 this 传入线程函数的原因：
         *      1、线程函数Thread::run是静态函数，无法直接访问类的成员变量
         *      2、通过将this指针作为参数传入，线程函数可以获得指向创建它的Thread对象的引用
         *      3、在run函数内部，通过(Thread *)arg将参数转换回Thread*指针，从而可以访问和操作该线程对象的成员变量
         */
        /**
         * 为什么线程函数需要设置为静态函数：
         *      1、pthread_create需要一个符合void* (*)(void*)签名的函数指针，而非静态成员函数隐含了一个this指针参数，
         *      实际签名是void* (Class::*)(void*)，与要求不符
         *      2、void* (*)(void*)表示一个指向“返回值为void*”“函数参数为void*”的函数的指针，即函数指针，void*表示可以指向任意类型的数据
         */
        int rt = pthread_create(&m_thread, nullptr, &Thread::run, this);
        if (rt)
        {
            SYLAR_LOG_ERROR(g_logger) << "pthread_create thread fail, rt = " << rt << " name = " << name;
            throw std::logic_error("pthread_create error");
        }

        m_semaphore.wait(); // 等待线程创建成功
    }
    Thread::~Thread()
    {
        // 当线程对象销毁时，如果线程仍处于活动状态，启动分离线程
        if (m_thread)
        {
            // 设置线程分离状态，当线程终止时自动回收资源
            pthread_detach(m_thread);
        }
    }
    pid_t Thread::getId() const { return m_id; }
    // 成员函数，依赖线程对象，返回某个线程对象的线程名
    const std::string &Thread::getName() const { return m_name; }
    // 静态函数，不依赖对象，可在任何地方调用，返回所在线程的线程名
    const std::string &Thread::GetName() { return t_thread_name; }
    // 获取当前线程的指针
    Thread *Thread::GetThis() { return t_thread; }

    void Thread::join()
    {
        if (m_thread)
        {
            int rt = pthread_join(m_thread, nullptr);
            if (rt)
            {
                SYLAR_LOG_ERROR(g_logger) << "pthread_join thread fail, rt = " << rt << " name = " << m_name;
                throw std::logic_error("pthread_join error");
            }
            m_thread = 0;
        }
    }
    /**
     * @brief 设置当前线程的名称
     * @param name 要设置的线程名称
     *
     * 1、首先检查当前线程对象是否存在(t_thread不为空)
     * 2、如果存在，则更新该线程对象的名称(m_name)
     * 3、同时更新线程局部存储的线程名称(t_thread_name)
     */
    void Thread::SetName(const std::string &name)
    {
        if (t_thread)
        {
            t_thread->m_name = name;
        }
        t_thread_name = name;
    }
    void *Thread::run(void *arg)
    {
        // 将传入的void*参数转换为Thread*类型指针
        // 这个arg实际上是pthread_create传入的this指针
        Thread *thread = (Thread *)arg;
        // 设置线程局部存储变量t_thread为当前线程对象
        // 这样在任何地方都可以通过Thread::GetThis()获取当前线程对象
        t_thread = thread;
        // 获取并设置当前线程的真实系统线程ID
        thread->m_id = sylar::GetThreadId();
        // 设置线程的名称，限制在15个字符以内
        pthread_setname_np(pthread_self(), thread->m_name.substr(0, 15).c_str());
        // 用于存储回调函数
        std::function<void()> cb;
        // 使用swap而不是直接赋值是为了避免增加智能指针的引用计数，swap的作用类似移交所属权
        // 同时swap操作后thread->m_cb会被清空，避免回调函数被重复执行
        cb.swap(thread->m_cb);        // 交换cb和thread->m_cb的内容
        thread->m_semaphore.notify(); // 通知主线程线程创建成功
        cb();                         // 执行回调函数
        return 0;                     // 结束线程
    }
}