#include "macro.hpp"
#include "scheduler.hpp"

auto g_logger = SYLAR_LOG_ROOT();

void test_fiber()
{
    static int count = 5;
    SYLAR_LOG_INFO(g_logger) << "test begin count=" << count;
    std::this_thread::sleep_for(std::chrono::seconds(1));
    if (--count>0)
    {
        sylar::Scheduler::GetThis()->schedule(test_fiber,sylar::GetThreadId());
    }
}

int main(int argc, char **argv)
{
    SYLAR_LOG_INFO(g_logger) << "main";
    sylar::Scheduler sc(3, false, "test");
    sc.start();
    SYLAR_LOG_INFO(g_logger) << "schedule";
    sc.schedule(test_fiber);
    SYLAR_LOG_INFO(g_logger) << "schedule end";
    sc.stop();
    SYLAR_LOG_INFO(g_logger) << "over";
    return 0;
}