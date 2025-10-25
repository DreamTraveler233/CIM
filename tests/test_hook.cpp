#include "hook.hpp"
#include "iomanager.hpp"
#include "logger.hpp"
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <time.h>

auto g_logger = CIM_LOG_ROOT();

void test_sleep()
{
    CIM::IOManager iom(1, false, "test");

    CIM::set_hook_enable(true);
    
    CIM_LOG_INFO(g_logger) << "test_sleep begin";
    time_t start = time(nullptr);
    
    iom.addTimer(1000, [](){
        CIM_LOG_INFO(g_logger) << "timer callback 1";
        sleep(2);
        CIM_LOG_INFO(g_logger) << "timer callback 1 end";
    });
    
    iom.addTimer(1500, [](){
        CIM_LOG_INFO(g_logger) << "timer callback 2";
        usleep(100000); // 100ms
        CIM_LOG_INFO(g_logger) << "timer callback 2 end";
    });
    
    time_t end = time(nullptr);
    CIM_LOG_INFO(g_logger) << "test_sleep end, cost time: " << (end - start) << "s";
}

int main(int argc, char** argv)
{
    test_sleep();
    return 0;
}