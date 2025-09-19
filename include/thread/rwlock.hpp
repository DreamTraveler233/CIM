#pragma once

#include "noncopyable.hpp"
#include <pthread.h>

namespace sylar
{
    // 通用锁管理模板，RAII风格
    template <class T>
    struct ScopedLockImpl : public noncopyable
    {
    public:
        // 构造的时候加锁
        ScopedLockImpl(T &mutex)
            : m_mutex(mutex)
        {
            m_mutex.lock();
            m_locked = true;
        }
        // 析构的时候解锁
        ~ScopedLockImpl()
        {
            unlock();
        }
        // 上锁（读锁/写锁）
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
        // 构造的时候加锁
        ReadScopedLockImpl(T &mutex)
            : m_mutex(mutex)
        {
            m_mutex.rdlock();
            m_locked = true;
        }
        // 析构的时候解锁
        ~ReadScopedLockImpl()
        {
            unlock();
        }
        // 主动加锁
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

    // 读共享、写独占
    class RWMutex : public noncopyable
    {
    public:
        using ReadLock = ReadScopedLockImpl<RWMutex>;
        using WriteLock = WriteScopedLockImpl<RWMutex>;

        RWMutex();
        ~RWMutex();
        // 读锁
        void rdlock();
        // 写锁
        void wrlock();
        // 解锁
        void unlock();

    private:
        pthread_rwlock_t m_lock;
    };
}