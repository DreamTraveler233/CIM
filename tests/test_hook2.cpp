#include "hook/hook.hpp"
#include "coroutine/iomanager.hpp"
#include "log/logger.hpp"
#include <unistd.h>
#include <time.h>

auto g_logger = SYLAR_LOG_ROOT();

void test_hook_with_coroutine() {
    sylar::IOManager iom(1, false, "test");
    
    SYLAR_LOG_INFO(g_logger) << "test_hook_with_coroutine begin";
    
    // 添加多个协程任务测试sleep/usleep
    iom.schedule([](){
        SYLAR_LOG_INFO(g_logger) << "coroutine 1 start";
        time_t start = time(nullptr);
        sleep(2);  // 应该不会阻塞其他协程
        time_t end = time(nullptr);
        SYLAR_LOG_INFO(g_logger) << "coroutine 1 end, cost: " << (end - start) << "s";
    });
    
    iom.schedule([](){
        SYLAR_LOG_INFO(g_logger) << "coroutine 2 start";
        time_t start = time(nullptr);
        usleep(3000000);  // 2000ms，应该很快完成
        time_t end = time(nullptr);
        SYLAR_LOG_INFO(g_logger) << "coroutine 2 end, cost: " << (end - start) << "s";
    });
    
    iom.schedule([](){
        SYLAR_LOG_INFO(g_logger) << "coroutine 3 start";
        // 这个协程不sleep，应该立即完成
        SYLAR_LOG_INFO(g_logger) << "coroutine 3 end";
    });
    
    SYLAR_LOG_INFO(g_logger) << "test_hook_with_coroutine end scheduling";
}

int main(int argc, char** argv)
{
    test_hook_with_coroutine();
    return 0;
}