#include "hook.hpp"
#include "iomanager.hpp"
#include "logger.hpp"
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>

auto g_logger = SYLAR_LOG_ROOT();

void test_hook_with_coroutine()
{
    CIM::IOManager iom(1, false, "test");

    SYLAR_LOG_INFO(g_logger) << "test_hook_with_coroutine begin";

    // 添加多个协程任务测试sleep/usleep
    iom.schedule([]()
                 {
        SYLAR_LOG_INFO(g_logger) << "coroutine 1 start";
        time_t start = time(nullptr);
        sleep(2);  // 应该不会阻塞其他协程
        time_t end = time(nullptr);
        SYLAR_LOG_INFO(g_logger) << "coroutine 1 end, cost: " << (end - start) << "s"; });

    iom.schedule([]()
                 {
        SYLAR_LOG_INFO(g_logger) << "coroutine 2 start";
        time_t start = time(nullptr);
        usleep(3000000);  // 2000ms，应该很快完成
        time_t end = time(nullptr);
        SYLAR_LOG_INFO(g_logger) << "coroutine 2 end, cost: " << (end - start) << "s"; });

    iom.schedule([]()
                 {
        SYLAR_LOG_INFO(g_logger) << "coroutine 3 start";
        // 这个协程不sleep，应该立即完成
        SYLAR_LOG_INFO(g_logger) << "coroutine 3 end"; });

    SYLAR_LOG_INFO(g_logger) << "test_hook_with_coroutine end scheduling";
}

void test_socket_hook()
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    inet_pton(AF_INET, "www.baidu.com", &addr.sin_addr.s_addr);

    SYLAR_LOG_INFO(g_logger) << "begin connect";
    int rt = connect(fd, (const sockaddr *)&addr, sizeof(addr));
    SYLAR_LOG_INFO(g_logger) << "connect rt=" << rt << " errno=" << errno;

    if (rt)
    {
        return;
    }

    const char data[] = "GET / HTTP/1.0\r\n\r\n";
    rt = send(fd, data, sizeof(data), 0);
    SYLAR_LOG_INFO(g_logger) << "send rt=" << rt << " errno=" << errno;

    if (rt <= 0)
    {
        return;
    }

    std::string buff;
    buff.resize(4096);

    rt = recv(fd, &buff[0], buff.size(), 0);
    SYLAR_LOG_INFO(g_logger) << "recv rt=" << rt << " errno=" << errno;

    if (rt <= 0)
    {
        return;
    }

    buff.resize(rt);
    SYLAR_LOG_INFO(g_logger) << buff;
}

int main(int argc, char **argv)
{
    // test_hook_with_coroutine();
    CIM::IOManager iom;
    iom.schedule(test_socket_hook);
    return 0;
}