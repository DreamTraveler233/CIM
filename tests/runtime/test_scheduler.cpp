#include "core/macro.hpp"
#include "runtime/scheduler.hpp"

auto g_logger = SYLAR_LOG_ROOT();

void test_fiber()
{
    static int count = 5;
    SYLAR_LOG_INFO(g_logger) << "test begin count=" << count;
    std::this_thread::sleep_for(std::chrono::seconds(1));
    if (--count>0)
    {
        sylar::Scheduler::GetThis()->schedule(test_fiber);
    }
}

int main(int argc, char **argv)
{
    sylar::Scheduler sc(2, true, "test");
    sc.start();
    sc.schedule(test_fiber);
    sc.stop();
    return 0;
}