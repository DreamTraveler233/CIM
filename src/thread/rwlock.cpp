#include "rwlock.hpp"

namespace sylar
{
    RWMutex::RWMutex()
    {
        // 初始化读写锁
        pthread_rwlock_init(&m_lock, nullptr);
    }
    RWMutex::~RWMutex()
    {
        // 销毁读写锁
        pthread_rwlock_destroy(&m_lock);
    }
    void RWMutex::rdlock()
    {
        // 获取读锁
        pthread_rwlock_rdlock(&m_lock);
    }
    void RWMutex::wrlock()
    {
        // 获取写锁
        pthread_rwlock_wrlock(&m_lock);
    }
    void RWMutex::unlock()
    {
        // 释放锁（无论读写锁）
        pthread_rwlock_unlock(&m_lock);
    }
}
