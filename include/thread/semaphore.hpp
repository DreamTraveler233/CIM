#pragma once

#include "noncopyable.hpp"
#include <stdint.h>
#include <semaphore.h>

namespace sylar
{
    class Semaphore : public Noncopyable
    {
    public:
        Semaphore(uint32_t count = 0);
        ~Semaphore();

        void wait();
        void notify();

    private:
        sem_t m_semaphore;
    };
}