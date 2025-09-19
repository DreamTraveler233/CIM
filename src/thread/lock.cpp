#include "lock.hpp"

namespace sylar
{
    RWMutex::RWMutex()
    {
        pthread_rwlock_init(&m_lock, nullptr);
    }
    RWMutex::~RWMutex()
    {
        pthread_rwlock_destroy(&m_lock);
    }
    void RWMutex::rlock()
    {
        pthread_rwlock_rdlock(&m_lock);
    }
    void RWMutex::wlock()
    {
        pthread_rwlock_wrlock(&m_lock);
    }
    void RWMutex::unlock()
    {
        pthread_rwlock_unlock(&m_lock);
    }
}
