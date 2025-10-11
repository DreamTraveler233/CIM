#include "net/socket.hpp"
#include "net/address.hpp"
#include "log/logger.hpp"
#include "log/logger_manager.hpp"
#include "runtime/iomanager.hpp"
#include <iostream>
#include <cstring>

static auto g_logger = SYLAR_LOG_ROOT();

void test_socket()
{
    // 使用DNS解析将域名 "www.baidu.com" 转换为IP地址
    sylar::IPAddress::ptr addr = sylar::Address::LookupAnyIpAddress("www.baidu.com");
    if (addr)
    {
        SYLAR_LOG_INFO(g_logger) << "get address: " << addr->toString();
    }
    else
    {
        SYLAR_LOG_INFO(g_logger) << "get address fail";
        return;
    }

    // 创建TCP套接字并连接到目标地址的80端口
    sylar::Socket::ptr sock = sylar::Socket::CreateTCP(addr);
    addr->setPort(80);
    if (!sock->connect(addr))
    {
        SYLAR_LOG_INFO(g_logger) << "connect " << addr->toString() << " fail";
        return;
    }
    else
    {
        SYLAR_LOG_INFO(g_logger) << "connect " << addr->toString() << " success";
    }

    // 发送HTTP GET请求到服务器
    const char data[] = "GET / HTTP/1.0\r\n\r\n";
    int rt = sock->send(data, sizeof(data));
    if (rt <= 0)
    {
        SYLAR_LOG_INFO(g_logger) << "send fail rt=" << rt;
        return;
    }
    else
    {
        SYLAR_LOG_INFO(g_logger) << "send " << rt << " bytes data";
    }

    // 接收并显示服务器的HTTP响应
    std::string buff;
    buff.resize(4096);
    rt = sock->recv(&buff[0], buff.size());
    if (rt <= 0)
    {
        SYLAR_LOG_INFO(g_logger) << "recv fail rt=" << rt;
        return;
    }
    else
    {
        SYLAR_LOG_INFO(g_logger) << "recv " << rt << " bytes data";
        buff.resize(rt);
        std::cout << buff << std::endl;
    }
}

int main(int argc, char **argv)
{
    sylar::IOManager iom;
    iom.schedule(test_socket);
    return 0;
}