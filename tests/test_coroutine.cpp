#include "macro.hpp"
#include "coroutine.hpp"

static auto g_logger = SYLAR_LOG_ROOT();

void run_in_coroutine()
{
    SYLAR_LOG_INFO(g_logger) << "run in coroutine begin";
    sylar::Coroutine::GetThis()->YieldToHold(); // 从子协程返回主协程
    SYLAR_LOG_INFO(g_logger) << "run in coroutine end";
    sylar::Coroutine::GetThis()->YieldToHold();
}

void test_fiber()
{
    // 创建当前线程的主协程
    sylar::Coroutine::GetThis();
    SYLAR_LOG_INFO(g_logger) << "main begin -1";
    {
        SYLAR_LOG_INFO(g_logger) << "main begin";
        // 创建子协程
        sylar::Coroutine::ptr coroutine = std::make_shared<sylar::Coroutine>(run_in_coroutine);
        coroutine->swapIn(); // 从当前线程的协程（主协程）进入子协程
        SYLAR_LOG_INFO(g_logger) << "main after swapIn";
        coroutine->swapIn(); // 进入子协程
        SYLAR_LOG_INFO(g_logger) << "main end";
        coroutine->swapIn();
    }
    SYLAR_LOG_INFO(g_logger) << "main after 2";
}

int main(int argc, char **argv)
{
    sylar::Thread::SetName("main");
    SYLAR_LOG_INFO(g_logger) << "main";
    std::vector<sylar::Thread::ptr> thrs;
    for(int i = 0; i < 5; ++i)
    {
        thrs.push_back(std::make_shared<sylar::Thread>(&test_fiber,"thread_"+std::to_string(i)));
    }
    for(auto i : thrs)
    {
        i->join();
    }
    return 0;
}