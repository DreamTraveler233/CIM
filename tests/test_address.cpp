#include "address.hpp"
#include "logger.hpp"
#include "logger_manager.hpp"
#include <iostream>
#include <vector>
#include <cassert>

static auto g_logger = CIM_LOG_ROOT();

void testIPv4Address() {
    CIM_LOG_INFO(g_logger) << "=== IPv4 Address Tests ===";
    
    // 测试默认构造函数
    CIM::IPv4Address addr1;
    CIM_LOG_INFO(g_logger) << "Default IPv4 address: " << addr1.toString();
    assert(addr1.toString() == "0.0.0.0:0");
    
    // 测试带参数构造函数
    CIM::IPv4Address addr2(0x01020304, 9999); // 1.2.3.4:9999
    CIM_LOG_INFO(g_logger) << "IPv4 address with params: " << addr2.toString();
    assert(addr2.toString() == "1.2.3.4:9999");
    
    // 测试端口设置和获取
    addr1.setPort(1234);
    CIM_LOG_INFO(g_logger) << "IPv4 address after setPort: " << addr1.toString() 
                             << ", getPort: " << addr1.getPort();
    assert(addr1.getPort() == 1234);
    assert(addr1.toString() == "0.0.0.0:1234");
    
    // 测试Create方法创建IPv4地址
    auto addr3 = CIM::IPv4Address::Create("192.168.1.10", 8080);
    if (addr3) {
        CIM_LOG_INFO(g_logger) << "Created IPv4 address: " << addr3->toString();
        assert(addr3->toString() == "192.168.1.10:8080");
        assert(addr3->getPort() == 8080);
    } else {
        CIM_LOG_ERROR(g_logger) << "Failed to create IPv4 address";
        assert(false);
    }
    
    // 测试网络地址计算
    auto subnet_mask = addr3->subnetMask(24);
    if (subnet_mask) {
        CIM_LOG_INFO(g_logger) << "Subnet mask for /24: " << subnet_mask->toString();
        assert(subnet_mask->toString() == "255.255.255.0:0");
    } else {
        CIM_LOG_ERROR(g_logger) << "Failed to create subnet mask";
        assert(false);
    }
    
    auto network_addr = addr3->networkAddress(24);
    if (network_addr) {
        CIM_LOG_INFO(g_logger) << "Network address for /24: " << network_addr->toString();
        assert(network_addr->toString() == "192.168.1.0:8080");
    } else {
        CIM_LOG_ERROR(g_logger) << "Failed to create network address";
        assert(false);
    }
    
    auto broadcast_addr = addr3->broadcastAddress(24);
    if (broadcast_addr) {
        CIM_LOG_INFO(g_logger) << "Broadcast address for /24: " << broadcast_addr->toString();
        assert(broadcast_addr->toString() == "192.168.1.255:8080");
    } else {
        CIM_LOG_ERROR(g_logger) << "Failed to create broadcast address";
        assert(false);
    }
    
    // 测试边界情况：/32 子网
    auto subnet_mask_32 = addr3->subnetMask(32);
    if (subnet_mask_32) {
        CIM_LOG_INFO(g_logger) << "Subnet mask for /32: " << subnet_mask_32->toString();
        assert(subnet_mask_32->toString() == "255.255.255.255:0");
    }
    
    auto network_addr_32 = addr3->networkAddress(32);
    if (network_addr_32) {
        CIM_LOG_INFO(g_logger) << "Network address for /32: " << network_addr_32->toString();
        assert(network_addr_32->toString() == "192.168.1.10:8080");
    }
    
    // 测试边界情况：/0 子网
    auto subnet_mask_0 = addr3->subnetMask(0);
    if (subnet_mask_0) {
        CIM_LOG_INFO(g_logger) << "Subnet mask for /0: " << subnet_mask_0->toString();
        assert(subnet_mask_0->toString() == "0.0.0.0:0");
    }
    
    auto network_addr_0 = addr3->networkAddress(0);
    if (network_addr_0) {
        CIM_LOG_INFO(g_logger) << "Network address for /0: " << network_addr_0->toString();
        assert(network_addr_0->toString() == "0.0.0.0:8080");
    }
    
    // 测试无效的前缀长度
    auto invalid_subnet = addr3->subnetMask(33);
    assert(invalid_subnet == nullptr);
    
    auto invalid_network = addr3->networkAddress(33);
    assert(invalid_network == nullptr);
    
    auto invalid_broadcast = addr3->broadcastAddress(33);
    assert(invalid_broadcast == nullptr);
}

void testIPv6Address() {
    CIM_LOG_INFO(g_logger) << "=== IPv6 Address Tests ===";
    
    // 测试默认构造函数
    CIM::IPv6Address addr1;
    CIM_LOG_INFO(g_logger) << "Default IPv6 address: " << addr1.toString();
    assert(addr1.toString() == "[::]:0");
    
    // 测试端口设置和获取
    addr1.setPort(5678);
    CIM_LOG_INFO(g_logger) << "IPv6 address after setPort: " << addr1.toString()
                             << ", getPort: " << addr1.getPort();
    assert(addr1.getPort() == 5678);
    assert(addr1.toString() == "[::]:5678");
    
    // 测试Create方法创建IPv6地址
    auto addr2 = CIM::IPv6Address::Create("::1", 8080);
    if (addr2) {
        CIM_LOG_INFO(g_logger) << "Created IPv6 address: " << addr2->toString();
        assert(addr2->getPort() == 8080);
    } else {
        CIM_LOG_ERROR(g_logger) << "Failed to create IPv6 address";
        assert(false);
    }
    
    // 测试Create方法创建IPv6地址（完整格式）
    auto addr3 = CIM::IPv6Address::Create("2001:0db8:85a3:0000:0000:8a2e:0370:7334", 9999);
    if (addr3) {
        CIM_LOG_INFO(g_logger) << "Created IPv6 address: " << addr3->toString();
        assert(addr3->getPort() == 9999);
    } else {
        CIM_LOG_ERROR(g_logger) << "Failed to create IPv6 address";
        assert(false);
    }
    
    // 测试网络地址计算
    auto subnet_mask = addr3->subnetMask(64);
    if (subnet_mask) {
        CIM_LOG_INFO(g_logger) << "Subnet mask for /64: " << subnet_mask->toString();
    }
    
    auto network_addr = addr3->networkAddress(64);
    if (network_addr) {
        CIM_LOG_INFO(g_logger) << "Network address for /64: " << network_addr->toString();
    }
    
    auto broadcast_addr = addr3->broadcastAddress(64);
    if (broadcast_addr) {
        CIM_LOG_INFO(g_logger) << "Broadcast address for /64: " << broadcast_addr->toString();
    }
    
    // 测试边界情况：/128 子网
    auto subnet_mask_128 = addr3->subnetMask(128);
    if (subnet_mask_128) {
        CIM_LOG_INFO(g_logger) << "Subnet mask for /128: " << subnet_mask_128->toString();
    }
    
    auto network_addr_128 = addr3->networkAddress(128);
    if (network_addr_128) {
        CIM_LOG_INFO(g_logger) << "Network address for /128: " << network_addr_128->toString();
    }
    
    // 测试边界情况：/0 子网
    auto subnet_mask_0 = addr3->subnetMask(0);
    if (subnet_mask_0) {
        CIM_LOG_INFO(g_logger) << "Subnet mask for /0: " << subnet_mask_0->toString();
    }
    
    auto network_addr_0 = addr3->networkAddress(0);
    if (network_addr_0) {
        CIM_LOG_INFO(g_logger) << "Network address for /0: " << network_addr_0->toString();
    }
    
    // 测试无效的前缀长度
    auto invalid_subnet = addr3->subnetMask(129);
    assert(invalid_subnet == nullptr);
    
    auto invalid_network = addr3->networkAddress(129);
    assert(invalid_network == nullptr);
    
    auto invalid_broadcast = addr3->broadcastAddress(129);
    assert(invalid_broadcast == nullptr);
}

void testAddressLookup() {
    CIM_LOG_INFO(g_logger) << "=== Address Lookup Tests ===";
    
    // 测试域名解析
    std::vector<CIM::Address::ptr> results;
    if (CIM::Address::Lookup(results, "localhost")) {
        CIM_LOG_INFO(g_logger) << "Lookup localhost: found " << results.size() << " addresses";
        for (size_t i = 0; i < results.size(); ++i) {
            CIM_LOG_INFO(g_logger) << "  [" << i << "] " << results[i]->toString();
        }
    } else {
        CIM_LOG_ERROR(g_logger) << "Failed to lookup localhost";
    }
    
    // 测试LookupAny
    auto addr_any = CIM::Address::LookupAny("127.0.0.1:3000");
    if (addr_any) {
        CIM_LOG_INFO(g_logger) << "LookupAny 127.0.0.1:3000: " << addr_any->toString();
    }
    
    // 测试LookupAnyIpAddress
    auto ip_addr = CIM::Address::LookupAnyIpAddress("127.0.0.1");
    if (ip_addr) {
        CIM_LOG_INFO(g_logger) << "LookupAnyIpAddress 127.0.0.1: " << ip_addr->toString();
    }
    
    // 测试IPv6地址解析
    auto ipv6_addr = CIM::Address::LookupAny("[::1]:8080");
    if (ipv6_addr) {
        CIM_LOG_INFO(g_logger) << "Lookup IPv6 [::1]:8080: " << ipv6_addr->toString();
    }
}

void testAddressComparison() {
    CIM_LOG_INFO(g_logger) << "=== Address Comparison Tests ===";
    
    auto addr1 = CIM::IPv4Address::Create("192.168.1.10", 8080);
    auto addr2 = CIM::IPv4Address::Create("192.168.1.10", 8080);
    auto addr3 = CIM::IPv4Address::Create("192.168.1.11", 8080);
    auto addr4 = CIM::IPv4Address::Create("192.168.1.10", 9090);
    
    if (addr1 && addr2 && addr3 && addr4) {
        CIM_LOG_INFO(g_logger) << "Address1: " << addr1->toString();
        CIM_LOG_INFO(g_logger) << "Address2: " << addr2->toString();
        CIM_LOG_INFO(g_logger) << "Address3: " << addr3->toString();
        CIM_LOG_INFO(g_logger) << "Address4: " << addr4->toString();
        
        // 测试相等性
        assert(*addr1 == *addr2);
        CIM_LOG_INFO(g_logger) << "addr1 == addr2: " << (*addr1 == *addr2);
        
        // 测试不等性
        assert(*addr1 != *addr3);
        CIM_LOG_INFO(g_logger) << "addr1 != addr3: " << (*addr1 != *addr3);
        
        assert(*addr1 != *addr4);
        CIM_LOG_INFO(g_logger) << "addr1 != addr4: " << (*addr1 != *addr4);
        
        // 测试小于比较
        assert(*addr1 < *addr3);
        CIM_LOG_INFO(g_logger) << "addr1 < addr3: " << (*addr1 < *addr3);
        
        // 测试IPv4与IPv6比较
        auto ipv6_addr = CIM::IPv6Address::Create("::1", 8080);
        if (ipv6_addr) {
            assert(*addr1 != *ipv6_addr);
            CIM_LOG_INFO(g_logger) << "IPv4 != IPv6: " << (*addr1 != *ipv6_addr);
        }
    }
}

void testAddressFactory() {
    CIM_LOG_INFO(g_logger) << "=== Address Factory Tests ===";
    
    // 测试IPv4地址创建
    sockaddr_in test_addr;
    memset(&test_addr, 0, sizeof(test_addr));
    test_addr.sin_family = AF_INET;
    test_addr.sin_port = htons(9999);
    test_addr.sin_addr.s_addr = htonl(0x01020304); // 1.2.3.4
    
    auto factory_addr = CIM::Address::Create((sockaddr*)&test_addr, sizeof(test_addr));
    if (factory_addr) {
        CIM_LOG_INFO(g_logger) << "Factory created IPv4 address: " << factory_addr->toString();
        assert(factory_addr->toString() == "1.2.3.4:9999");
    }
    
    // 测试IPv6地址创建
    sockaddr_in6 test_addr6;
    memset(&test_addr6, 0, sizeof(test_addr6));
    test_addr6.sin6_family = AF_INET6;
    test_addr6.sin6_port = htons(8888);
    test_addr6.sin6_addr = in6addr_loopback; // ::1
    
    auto factory_addr6 = CIM::Address::Create((sockaddr*)&test_addr6, sizeof(test_addr6));
    if (factory_addr6) {
        CIM_LOG_INFO(g_logger) << "Factory created IPv6 address: " << factory_addr6->toString();
    }
    
    // 测试未知地址类型
    sockaddr test_unknown;
    memset(&test_unknown, 0, sizeof(test_unknown));
    test_unknown.sa_family = AF_UNIX; // 使用Unix域套接字族作为未知类型示例
    
    auto factory_unknown = CIM::Address::Create((sockaddr*)&test_unknown, sizeof(test_unknown));
    if (factory_unknown) {
        CIM_LOG_INFO(g_logger) << "Factory created unknown address: " << factory_unknown->toString();
    }
}

void testInterfaceAddresses() {
    CIM_LOG_INFO(g_logger) << "=== Interface Address Tests ===";
    
    // 测试获取所有接口地址
    std::multimap<std::string, std::pair<CIM::Address::ptr, uint32_t>> all_iface_addrs;
    if (CIM::Address::GetInterfaceAddresses(all_iface_addrs)) {
        CIM_LOG_INFO(g_logger) << "All interface addresses (" << all_iface_addrs.size() << " entries):";
        for (const auto& pair : all_iface_addrs) {
            CIM_LOG_INFO(g_logger) << "  " << pair.first << ": " << pair.second.first->toString()
                                     << " prefix_len: " << pair.second.second;
        }
    } else {
        CIM_LOG_INFO(g_logger) << "Failed to get all interface addresses";
    }
    
    // 测试获取特定接口地址 (lo)
    std::vector<std::pair<CIM::Address::ptr, uint32_t>> iface_addrs;
    if (CIM::Address::GetInterfaceAddresses(iface_addrs, "lo")) {
        CIM_LOG_INFO(g_logger) << "Interface 'lo' addresses:";
        for (size_t i = 0; i < iface_addrs.size(); ++i) {
            CIM_LOG_INFO(g_logger) << "  [" << i << "] " << iface_addrs[i].first->toString()
                                     << " prefix_len: " << iface_addrs[i].second;
        }
    } else {
        CIM_LOG_INFO(g_logger) << "No addresses found for interface 'lo' or interface not found";
    }
    
    // 测试获取默认接口地址
    std::vector<std::pair<CIM::Address::ptr, uint32_t>> default_addrs;
    if (CIM::Address::GetInterfaceAddresses(default_addrs, "")) {
        CIM_LOG_INFO(g_logger) << "Default interface addresses:";
        for (size_t i = 0; i < default_addrs.size(); ++i) {
            CIM_LOG_INFO(g_logger) << "  [" << i << "] " << default_addrs[i].first->toString()
                                     << " prefix_len: " << default_addrs[i].second;
        }
    }
    
    // 测试获取通配符接口地址
    std::vector<std::pair<CIM::Address::ptr, uint32_t>> wildcard_addrs;
    if (CIM::Address::GetInterfaceAddresses(wildcard_addrs, "*")) {
        CIM_LOG_INFO(g_logger) << "Wildcard interface addresses:";
        for (size_t i = 0; i < wildcard_addrs.size(); ++i) {
            CIM_LOG_INFO(g_logger) << "  [" << i << "] " << wildcard_addrs[i].first->toString()
                                     << " prefix_len: " << wildcard_addrs[i].second;
        }
    }
}

void testIPAddressFactory() {
    CIM_LOG_INFO(g_logger) << "=== IPAddress Factory Tests ===";
    
    // 测试IPv4地址创建
    auto ip_addr1 = CIM::IPAddress::Create("192.168.1.1", 1234);
    if (ip_addr1) {
        CIM_LOG_INFO(g_logger) << "IPAddress::Create IPv4: " << ip_addr1->toString();
        assert(ip_addr1->toString() == "192.168.1.1:1234");
    }
    
    // 测试IPv6地址创建
    auto ip_addr2 = CIM::IPAddress::Create("::1", 5678);
    if (ip_addr2) {
        CIM_LOG_INFO(g_logger) << "IPAddress::Create IPv6: " << ip_addr2->toString();
    }
}

int main(int argc, char **argv)
{
    CIM_LOG_INFO(g_logger) << "Starting address tests...";
    
    try {
        testIPv4Address();
        testIPv6Address();
        testAddressLookup();
        testAddressComparison();
        testAddressFactory();
        testInterfaceAddresses();
        testIPAddressFactory();
        
        CIM_LOG_INFO(g_logger) << "All tests passed!";
    } catch (const std::exception& e) {
        CIM_LOG_ERROR(g_logger) << "Test failed with exception: " << e.what();
        return 1;
    } catch (...) {
        CIM_LOG_ERROR(g_logger) << "Test failed with unknown exception";
        return 1;
    }
    
    return 0;
}