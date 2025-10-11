#include "net/address.hpp"
#include "log/logger.hpp"
#include "log/logger_manager.hpp"
#include <iostream>
#include <vector>

static auto g_logger = SYLAR_LOG_ROOT();

int main(int argc, char **argv)
{
    // 测试IPv4地址创建和基本功能
    SYLAR_LOG_INFO(g_logger) << "=== IPv4 Address Tests ===";
    
    // 1. 测试Create方法创建IPv4地址
    auto addr1 = sylar::IPv4Address::Create("192.168.1.10", 8080);
    if (addr1) {
        SYLAR_LOG_INFO(g_logger) << "Created IPv4 address: " << addr1->toString();
    } else {
        SYLAR_LOG_ERROR(g_logger) << "Failed to create IPv4 address";
    }
    
    // 2. 测试默认构造函数
    sylar::IPv4Address addr2;
    SYLAR_LOG_INFO(g_logger) << "Default IPv4 address: " << addr2.toString();
    
    // 3. 测试带参数构造函数
    sylar::IPv4Address addr3(0x01020304, 9999); // 1.2.3.4:9999
    SYLAR_LOG_INFO(g_logger) << "IPv4 address with params: " << addr3.toString();
    
    // 4. 测试端口设置和获取
    addr2.setPort(1234);
    SYLAR_LOG_INFO(g_logger) << "IPv4 address after setPort: " << addr2.toString() 
                             << ", getPort: " << addr2.getPort();
    
    // 5. 测试网络地址计算
    auto subnet_mask = addr1->subnetMask(24);
    if (subnet_mask) {
        SYLAR_LOG_INFO(g_logger) << "Subnet mask for /24: " << subnet_mask->toString();
    }
    
    auto network_addr = addr1->networkAddress(24);
    if (network_addr) {
        SYLAR_LOG_INFO(g_logger) << "Network address for /24: " << network_addr->toString();
    }
    
    auto broadcast_addr = addr1->broadcastAddress(24);
    if (broadcast_addr) {
        SYLAR_LOG_INFO(g_logger) << "Broadcast address for /24: " << broadcast_addr->toString();
    }
    
    // 测试IPv6地址创建和基本功能
    SYLAR_LOG_INFO(g_logger) << "=== IPv6 Address Tests ===";
    
    // 1. 测试Create方法创建IPv6地址
    auto addr4 = sylar::IPv6Address::Create("::1", 8080);
    if (addr4) {
        SYLAR_LOG_INFO(g_logger) << "Created IPv6 address: " << addr4->toString();
    } else {
        SYLAR_LOG_ERROR(g_logger) << "Failed to create IPv6 address";
    }
    
    // 2. 测试默认构造函数
    sylar::IPv6Address addr5;
    SYLAR_LOG_INFO(g_logger) << "Default IPv6 address: " << addr5.toString();
    
    // 3. 测试端口设置和获取
    addr5.setPort(5678);
    SYLAR_LOG_INFO(g_logger) << "IPv6 address after setPort: " << addr5.toString()
                             << ", getPort: " << addr5.getPort();
    
    // 测试Address类的Lookup功能
    SYLAR_LOG_INFO(g_logger) << "=== Address Lookup Tests ===";
    
    // 1. 测试域名解析
    std::vector<sylar::Address::ptr> results;
    if (sylar::Address::Lookup(results, "localhost")) {
        SYLAR_LOG_INFO(g_logger) << "Lookup localhost: found " << results.size() << " addresses";
        for (size_t i = 0; i < results.size(); ++i) {
            SYLAR_LOG_INFO(g_logger) << "  [" << i << "] " << results[i]->toString();
        }
    } else {
        SYLAR_LOG_ERROR(g_logger) << "Failed to lookup localhost";
    }
    
    // 2. 测试LookupAny
    auto addr_any = sylar::Address::LookupAny("127.0.0.1:3000");
    if (addr_any) {
        SYLAR_LOG_INFO(g_logger) << "LookupAny 127.0.0.1:3000: " << addr_any->toString();
    }
    
    // 3. 测试LookupAnyIpAddress
    auto ip_addr = sylar::Address::LookupAnyIpAddress("127.0.0.1");
    if (ip_addr) {
        SYLAR_LOG_INFO(g_logger) << "LookupAnyIpAddress 127.0.0.1: " << ip_addr->toString();
    }
    
    // 测试Address比较操作符
    SYLAR_LOG_INFO(g_logger) << "=== Address Comparison Tests ===";
    
    auto addr_comp1 = sylar::IPv4Address::Create("192.168.1.10", 8080);
    auto addr_comp2 = sylar::IPv4Address::Create("192.168.1.10", 8080);
    auto addr_comp3 = sylar::IPv4Address::Create("192.168.1.11", 8080);
    
    if (addr_comp1 && addr_comp2 && addr_comp3) {
        SYLAR_LOG_INFO(g_logger) << "Address1: " << addr_comp1->toString();
        SYLAR_LOG_INFO(g_logger) << "Address2: " << addr_comp2->toString();
        SYLAR_LOG_INFO(g_logger) << "Address3: " << addr_comp3->toString();
        
        SYLAR_LOG_INFO(g_logger) << "addr1 == addr2: " << (*addr_comp1 == *addr_comp2);
        SYLAR_LOG_INFO(g_logger) << "addr1 != addr3: " << (*addr_comp1 != *addr_comp3);
        SYLAR_LOG_INFO(g_logger) << "addr1 < addr3: " << (*addr_comp1 < *addr_comp3);
    }
    
    // 测试Address::Create工厂方法
    SYLAR_LOG_INFO(g_logger) << "=== Address Factory Tests ===";
    
    sockaddr_in test_addr;
    memset(&test_addr, 0, sizeof(test_addr));
    test_addr.sin_family = AF_INET;
    test_addr.sin_port = htons(9999);
    test_addr.sin_addr.s_addr = htonl(0x01020304); // 1.2.3.4
    
    auto factory_addr = sylar::Address::Create((sockaddr*)&test_addr, sizeof(test_addr));
    if (factory_addr) {
        SYLAR_LOG_INFO(g_logger) << "Factory created address: " << factory_addr->toString();
    }
    
    // 测试接口地址获取
    SYLAR_LOG_INFO(g_logger) << "=== Interface Address Tests ===";
    
    std::vector<std::pair<sylar::Address::ptr, uint32_t>> iface_addrs;
    if (sylar::Address::GetInterfaceAddresses(iface_addrs, "lo")) {
        SYLAR_LOG_INFO(g_logger) << "Interface 'lo' addresses:";
        for (size_t i = 0; i < iface_addrs.size(); ++i) {
            SYLAR_LOG_INFO(g_logger) << "  [" << i << "] " << iface_addrs[i].first->toString()
                                     << " prefix_len: " << iface_addrs[i].second;
        }
    } else {
        SYLAR_LOG_INFO(g_logger) << "No addresses found for interface 'lo' or interface not found";
    }
    
    return 0;
}