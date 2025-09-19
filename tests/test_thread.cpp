#include "thread.hpp"
#include "log.hpp"
#include <vector>

auto g_logger = SYLAR_LOG_ROOT();

void func1()
{
    SYLAR_LOG_INFO(g_logger) << "name: " << sylar::Thread::GetName()
                             << "this.name: " << sylar::Thread::GetThis()->getName()
                             << " id: " << sylar::GetThreadId()
                             << " this.id: " << sylar::Thread::GetThis()->getId();
    // std::this_thread::sleep_for(std::chrono::seconds(360));
}

void func2()
{
}

int main(int argc, char **argv)
{
    SYLAR_LOG_INFO(g_logger) << "thread test begin";
    std::vector<sylar::Thread::ptr> thrs;
    for (int i = 0; i < 5; ++i)
    {
        sylar::Thread::ptr thr(new sylar::Thread(&func1, "name_" + std::to_string(i)));
        thrs.push_back(thr);
    }

    for (int i = 0; i < 5; ++i)
    {
        thrs[i]->join();
    }
    SYLAR_LOG_INFO(g_logger) << "thread test begin";
    return 0;
}