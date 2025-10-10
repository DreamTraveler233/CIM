#include "hook/hook.hpp"
#include "coroutine/iomanager.hpp"
#include "log/logger.hpp"
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <time.h>

auto g_logger = SYLAR_LOG_ROOT();

void test_sleep()
{
    sylar::IOManager iom(1, false, "test");

    sylar::set_hook_enable(true);
    
    SYLAR_LOG_INFO(g_logger) << "test_sleep begin";
    time_t start = time(nullptr);
    
    iom.addTimer(1000, [](){
        SYLAR_LOG_INFO(g_logger) << "timer callback 1";
        sleep(2);
        SYLAR_LOG_INFO(g_logger) << "timer callback 1 end";
    });
    
    iom.addTimer(1500, [](){
        SYLAR_LOG_INFO(g_logger) << "timer callback 2";
        usleep(100000); // 100ms
        SYLAR_LOG_INFO(g_logger) << "timer callback 2 end";
    });
    
    time_t end = time(nullptr);
    SYLAR_LOG_INFO(g_logger) << "test_sleep end, cost time: " << (end - start) << "s";
}

int main(int argc, char** argv)
{
    test_sleep();
    return 0;
}