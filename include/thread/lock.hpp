#pragma once

#include "noncopyable.hpp"
#include <pthread.h>

namespace sylar
{
    template <class T>
    struct ScopedLockImpl : public noncopyable
    {
    public:
        // 构造的时候加锁
        ScopedLockImpl(T &mutex)
            : m_mutex(mutex)
        {
            m_mutex.lock();
        }
        // 析构的时候解锁
        ~ScopedLockImpl()
        {
            unlock();
        }
        void lock()
        {
            if (!m_locked)
            {
                m_mutex.lock();
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
        bool m_locked = false;
    };

    template <class T>
    struct ReadScopedLockImpl : public noncopyable
    {
    public:
        // 构造的时候加锁
        ReadScopedLockImpl(T &mutex)
            : m_mutex(mutex)
        {
            m_mutex.rlock();
        }
        // 析构的时候解锁
        ~ReadScopedLockImpl()
        {
            unlock();
        }
        void lock()
        {
            if (!m_locked)
            {
                m_mutex.rlock();
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
        bool m_locked = false;
    };

    template <class T>
    struct WriteScopedLockImpl : public noncopyable
    {
    public:
        // 构造的时候加锁
        WriteScopedLockImpl(T &mutex)
            : m_mutex(mutex)
        {
            m_mutex.wrlock();
        }
        // 析构的时候解锁
        ~WriteScopedLockImpl()
        {
            unlock();
        }
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
        bool m_locked = false;
    };

    class RWMutex : public noncopyable
    {
    public:
        using ReadLock = ReadScopedLockImpl<RWMutex>;
        using WriteLock = WriteScopedLockImpl<RWMutex>;

        RWMutex();
        ~RWMutex();
        // 读锁
        void rlock();
        // 写锁
        void wlock();
        // 解锁
        void unlock();

    private:
        pthread_rwlock_t m_lock;
    };
}