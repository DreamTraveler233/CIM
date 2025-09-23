#pragma once

#include "noncopyable.hpp"
#include <pthread.h>
#include <atomic>

namespace sylar
{
    // 通用锁管理模板，RAII风格
    template <class T>
    struct ScopedLockImpl : public noncopyable
    {
    public:
        ScopedLockImpl(T &mutex)
            : m_mutex(mutex)
        {
            m_mutex.lock();
            m_locked = true;
        }
        // 构造的时候加锁，用于const上下文(const 函数)
        ScopedLockImpl(const T &mutex)
            : m_mutex(const_cast<T &>(mutex))
        {
            m_mutex.lock();
            m_locked = true;
        }
        ~ScopedLockImpl()
        {
            unlock();
        }
        // 手动上锁（读锁/写锁）
        void lock()
        {
            if (!m_locked)
            {
                m_mutex.lock();
                m_locked = true;
            }
        }
        // 解锁（读锁/写锁）
        void unlock()
        {
            if (m_locked)
            {
                m_mutex.unlock();
                m_locked = false;
            }
        }

    private:
        T &m_mutex;
        bool m_locked;
    };

    // 读锁
    template <class T>
    struct ReadScopedLockImpl : public noncopyable
    {
    public:
        ReadScopedLockImpl(T &mutex)
            : m_mutex(mutex)
        {
            m_mutex.rdlock();
            m_locked = true;
        }
        // 构造的时候加锁，用于const上下文
        ReadScopedLockImpl(const T &mutex)
            : m_mutex(const_cast<T &>(mutex))
        {
            m_mutex.rdlock();
            m_locked = true;
        }
        ~ReadScopedLockImpl()
        {
            unlock();
        }
        void lock()
        {
            if (!m_locked)
            {
                m_mutex.rdlock();
                m_locked = true;
            }
        }
        void unlock()
        {
            if (m_locked)
            {
                m_mutex.unlock();
                m_locked = false;
            }
        }

    private:
        T &m_mutex;
        bool m_locked;
    };

    // 写锁
    template <class T>
    struct WriteScopedLockImpl : public noncopyable
    {
    public:
        // 构造的时候加锁
        WriteScopedLockImpl(T &mutex)
            : m_mutex(mutex)
        {
            m_mutex.wrlock();
            m_locked = true;
        }

        // 构造的时候加锁，用于const上下文
        WriteScopedLockImpl(const T &mutex)
            : m_mutex(const_cast<T &>(mutex))
        {
            m_mutex.wrlock();
            m_locked = true;
        }

        // 析构的时候解锁
        ~WriteScopedLockImpl()
        {
            unlock();
        }
        // 主动加锁
        void lock()
        {
            if (!m_locked)
            {
                m_mutex.wrlock();
                m_locked = true;
            }
        }
        void unlock()
        {
            if (m_locked)
            {
                m_mutex.unlock();
                m_locked = false;
            }
        }

    private:
        T &m_mutex;
        bool m_locked;
    };

    class Mutex : public noncopyable
    {
    public:
        using Lock = ScopedLockImpl<Mutex>;

        Mutex();
        ~Mutex();

        void lock();
        void unlock();

    private:
        pthread_mutex_t m_mutex;
    };

    // 读共享、写独占
    class RWMutex : public noncopyable
    {
    public:
        using ReadLock = ReadScopedLockImpl<RWMutex>;
        using WriteLock = WriteScopedLockImpl<RWMutex>;

        RWMutex();
        ~RWMutex();

        void rdlock();
        void wrlock();
        void unlock();

    private:
        pthread_rwlock_t m_mutex;
    };

    // 自旋锁
    class SpinLock : public noncopyable
    {
    public:
        using Lock = ScopedLockImpl<SpinLock>;

        SpinLock();
        ~SpinLock();

        void lock();
        void unlock();

    private:
        pthread_spinlock_t m_mutex;
    };

    class CASLock : public noncopyable
    {
    public:
        using Lock = ScopedLockImpl<CASLock>;
        
        CASLock();
        ~CASLock();

        void lock();
        void unlock();

    private:
        volatile std::atomic_flag m_mutex;
    };

    class NullMutex : public noncopyable
    {
    public:
        using Lock = ScopedLockImpl<NullMutex>;

        NullMutex() {}
        ~NullMutex() {}

        void lock() {}
        void unlock() {}
    };

    class NullRWMutex : public noncopyable
    {
    public:
        using ReadLock = ReadScopedLockImpl<NullRWMutex>;
        using WriteLock = WriteScopedLockImpl<NullRWMutex>;

        NullRWMutex() {}
        ~NullRWMutex() {}

        void rdlock() {}
        void wrlock() {}
        void unlock() {}
    };

    /**
     * 互斥锁的应用场景：
     *      完全互斥的访问：当多个线程都需要对同一资源进行读写操作时，使用互斥锁确保任何时刻只有一个线程能访问该资源。
     *      写多读少的场景：如果对资源的访问大部分是写操作，读写比例接近1:1，使用互斥锁更为合适。
     *      简单的同步需求：对于简单的临界区保护，互斥锁是最佳选择，因为它实现简单，开销较小。
     * 独写锁的应用场景：
     *      读多写少的场景：当对资源的访问以读操作为主（读写比例远大于1:1），使用读写锁可以大大提高并发性能。
     *      可并发读取的资源：允许多个线程同时读取资源，但写操作需要独占访问。
     *      提高读操作并发性：在读操作不会改变资源状态的情况下，多个读操作可以同时进行。
     * 在哪些地方需要加锁：
     *      保护共享资源的访问：当多个线程需要访问和修改同一个全局变量或共享资源时，需要使用锁来保护。
     *      临界区保护：在执行原子操作或需要保证一系列操作不被其他线程中断时，需要使用锁。
     *      容器操作：当多个线程同时读写STL容器或其他非线程安全的数据结构时。
     */
}