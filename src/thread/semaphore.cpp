#include "semaphore.hpp"
#include <stdexcept>

namespace sylar
{
    /**
     * @brief 构造函数，初始化信号量
     * @param count 信号量的初始值
     */
    Semaphore::Semaphore(uint32_t count)
    {
        if (sem_init(&m_semaphore, 0, count))
        {
            throw std::logic_error("sem_init error");
        }
    }

    /**
     * @brief 析构函数，销毁信号量
     */
    Semaphore::~Semaphore()
    {
        sem_destroy(&m_semaphore);
    }

    /**
     * @brief 等待/获取信号量，P操作
     * @details 将信号量值减1，如果信号量值为0则阻塞等待直到其他线程释放信号量
     * @exception std::logic_error 当sem_wait系统调用失败时抛出
     */
    void Semaphore::wait()
    {
        if (sem_wait(&m_semaphore))
        {
            throw std::logic_error("sem_wait error");
        }
    }

    /**
     * @brief 释放信号量，V操作
     * @details 将信号量值加1，如果有其他线程正在等待该信号量，则唤醒其中一个线程
     * @exception std::logic_error 当sem_post系统调用失败时抛出
     */
    void Semaphore::notify()
    {
        if (sem_post(&m_semaphore))
        {
            throw std::logic_error("sem_post error");
        }
    }
}