#include "util.hpp"

pid_t sylar::GetThreadId()
{
    return syscall(SYS_gettid);
}

uint32_t sylar::GetFiberId()
{
    return 0;
}
