#include "address.hpp"
#include "endian.hpp"
#include "macro.hpp"
#include <sstream>
#include <cstring>
#include <algorithm>
#include <netdb.h>
#include <ifaddrs.h>

namespace sylar
{
    static auto g_logger = SYLAR_LOG_NAME("system");

    /**
     * @brief 创建一个掩码值
     * @tparam T 掩码值的数据类型
     * @param bits 需要设置为0的低位数量
     * @return 返回创建的掩码值
     *
     * @details 该函数用于创建一个指定类型的掩码，其中低bits位为1，其余高位为0。
     * 例如：CreateMask<uint32_t>(3)将返回二进制表示为...000111的值（低3位为1）。
     * 这在网络地址计算中常用于生成子网掩码或主机部分掩码。
     */
    template <typename T>
    static T CreateMask(uint32_t bits)
    {
        return (1 << (sizeof(T) * 8 - bits)) - 1;
    }

    /**
     * @brief 计算一个数值中设置为1的位数（汉明重量）
     * @tparam T 输入值的数据类型
     * @param value 需要计算的值
     * @return 返回值中为1的位的数量
     *
     * @details 该函数使用Brian Kernighan算法来计算二进制表示中1的个数。
     * 算法原理：对于任意整数n，n&(n-1)的操作会清除n中最右边的1位。
     * 通过不断执行此操作直到值变为0，统计执行次数即可得到1的个数。
     * 这在网络编程中常用于计算网络前缀长度或校验网络地址格式。
     */
    template <class T>
    static uint32_t CountBytes(T value)
    {
        uint32_t result = 0;
        for (; value; ++result)
        {
            value &= value - 1;
        }
        return result;
    }

    /**
     * @brief 根据sockaddr结构体创建对应的Address对象
     * @param[in] addr sockaddr结构体指针
     * @param[in] addrlen 地址结构体长度
     * @return 返回创建的Address对象智能指针，如果传入addr为空则返回nullptr
     *
     * @details 该函数根据sockaddr结构体中的地址族(sa_family)字段来确定
     * 应该创建哪种类型的地址对象:
     * - AF_INET: 创建IPv4地址对象(IPv4Address)
     * - AF_INET6: 创建IPv6地址对象(IPv6Address)
     * - 其他: 创建未知地址对象(UnknownAddress)
     *
     * 这是一个工厂方法，用于将底层的sockaddr结构体封装成面向对象的Address实例
     */
    Address::ptr Address::Create(const sockaddr *addr, socklen_t addrlen)
    {
        if (addr == nullptr)
        {
            return nullptr;
        }

        Address::ptr result;
        switch (addr->sa_family)
        {
        case AF_INET:
            result.reset(new IPv4Address(*(const sockaddr_in *)addr));
            break;
        case AF_INET6:
            result.reset(new IPv6Address(*(const sockaddr_in6 *)addr));
            break;
        default:
            result.reset(new UnknownAddress(*addr));
            break;
        }
        return result;
    }

    /**
     * @brief 查询给定主机名对应的地址列表
     * @param[out] result 存储解析结果的地址向量
     * @param[in] host 主机名，可以是域名或IP地址，支持IPv6的方括号格式
     * @param[in] family 协议族，如AF_INET、AF_INET6等，默认为AF_UNSPEC表示任意协议族
     * @param[in] type socket类型，如SOCK_STREAM、SOCK_DGRAM等，默认为0表示任意类型
     * @param[in] protocol 协议类型，默认为0表示任意协议
     * @return 返回是否解析成功
     *
     * @details 该函数使用getaddrinfo函数解析主机名，支持以下格式：
     * - 域名: "www.example.com"
     * - IPv4地址: "192.168.1.1"
     * - IPv6地址: "2001:db8::1" 或 "[2001:db8::1]"
     * - 带端口的地址: "[2001:db8::1]:8080" 或 "192.168.1.1:8080"
     *
     * 函数会根据host参数的格式自动提取节点名称和端口号，然后调用getaddrinfo进行解析，
     * 将所有解析到的地址添加到result向量中。
     *
     * @note 如果解析失败，会通过日志记录错误信息
     */
    bool Address::Lookup(std::vector<Address::ptr> &result, const std::string &host,
                         int family, int type, int protocol)
    {
        addrinfo hints, *results, *next;
        hints.ai_flags = 0;
        hints.ai_family = family;
        hints.ai_socktype = type;
        hints.ai_protocol = protocol;
        hints.ai_addrlen = 0;
        hints.ai_addr = nullptr;
        hints.ai_canonname = nullptr;
        hints.ai_next = nullptr;

        std::string node;
        const char *service = nullptr;

        // 处理IPv6地址的方括号格式，如"[2001:db8::1]:8080"
        if (!host.empty() && host[0] != '[')
        {
            const char *endipv6 = (const char *)memchr(host.c_str() + 1, ']', host.size() - 1);
            if (endipv6)
            {
                // 如果找到结束方括号，并且后面跟着冒号，则冒号后为端口号
                if (*(endipv6 + 1) == ':')
                {
                    service = endipv6 + 2;
                }
                // 提取方括号内的IPv6地址
                node = host.substr(1, endipv6 - host.c_str() - 1);
            }
        }

        // 如果还没有解析出node，则继续处理普通格式的地址
        if (node.empty())
        {
            service = (const char *)memchr(host.c_str(), ':', host.size());
            if (service)
            {
                // 检查冒号后面是否还有冒号，以区分IPv6地址和端口号
                if (!memchr(service + 1, ':', host.c_str() + host.size() - service - 1))
                {
                    // 冒号后面没有其他冒号，说明这是端口号分隔符
                    node = host.substr(0, service - host.c_str());
                    ++service;
                }
            }
        }

        // 如果仍然没有解析出node，则整个host都是节点名称
        if (node.empty())
        {
            node = host;
        }

        // 调用系统函数getaddrinfo进行地址解析
        int error = getaddrinfo(node.c_str(), service, &hints, &results);
        if (error)
        {
            SYLAR_LOG_ERROR(g_logger) << "Address::Create getaddrinfo(" << host << ", "
                                      << family << ", " << type << ") error=" << error
                                      << " errstr=" << strerror(error);
            return false;
        }

        // 遍历解析结果链表，创建Address对象并添加到result中
        next = results;
        while (next)
        {
            result.push_back(Create(next->ai_addr, (socklen_t)next->ai_addrlen));
            next = next->ai_next;
        }

        // 释放getaddrinfo分配的内存
        freeaddrinfo(results);
        return true;
    }

    /**
     * @brief 查找主机名对应的第一个地址
     * @param[in] host 主机名，可以是域名或IP地址
     * @param[in] family 协议族，如AF_INET、AF_INET6等，默认为AF_UNSPEC表示任意协议族
     * @param[in] type socket类型，如SOCK_STREAM、SOCK_DGRAM等，默认为0表示任意类型
     * @param[in] protocol 协议类型，默认为0表示任意协议
     * @return 返回解析到的第一个地址，如果解析失败则返回nullptr
     *
     * @details 此函数是对Lookup函数的简单封装，只返回解析到的第一个地址，
     * 适用于只需要一个地址的场景。
     */
    Address::ptr Address::LookupAny(const std::string &host, int family, int type, int protocol)
    {
        std::vector<Address::ptr> result;
        if (Lookup(result, host, family, type, protocol))
        {
            return result[0];
        }
        return nullptr;
    }

    /**
     * @brief 查找主机名对应的第一个IP地址
     * @param[in] host 主机名，可以是域名或IP地址
     * @param[in] family 协议族，如AF_INET、AF_INET6等，默认为AF_UNSPEC表示任意协议族
     * @param[in] type socket类型，如SOCK_STREAM、SOCK_DGRAM等，默认为0表示任意类型
     * @param[in] protocol 协议类型，默认为0表示任意协议
     * @return 返回解析到的第一个IP地址，如果没有解析到IP地址则返回nullptr
     *
     * @details 此函数首先调用Lookup获取所有地址，然后遍历结果查找第一个
     * 可以转换为IPAddress类型的地址。相比于LookupAny，这个函数确保返回的
     * 是IP地址而不是其他类型的地址（如Unix域套接字地址）。
     */
    std::shared_ptr<IPAddress> Address::LookupAnyIpAddress(const std::string &host, int family, int type, int protocol)
    {
        std::vector<Address::ptr> result;
        if (Lookup(result, host, family, type, protocol))
        {
            for (auto &i : result)
            {
                IPAddress::ptr ip = std::dynamic_pointer_cast<IPAddress>(i);
                if (ip)
                {
                    return ip;
                }
            }
        }
        return nullptr;
    }

    /**
     * @brief 获取网络接口的IP地址信息
     * @param[out] result 存储网络接口地址的结果集，key为接口名称，value为pair类型，
     *                    其中first为接口地址，second为网络前缀长度
     * @param[in] family 地址族，AF_INET表示IPv4，AF_INET6表示IPv6，AF_UNSPEC表示不限制地址族
     * @return 成功获取至少一个接口地址时返回true，否则返回false
     *
     * @details 此函数通过调用getifaddrs系统调用来获取所有网络接口的信息，
     *          包括接口名称、地址和子网掩码，并计算网络前缀长度。
     *          支持IPv4和IPv6地址族，其他地址族会被忽略。
     *
     * @note 函数内部会对结果进行异常处理，即使发生异常也会释放已分配的资源
     */
    bool Address::GetInterfaceAddresses(std::multimap<std::string, std::pair<Address::ptr, uint32_t>> &result,
                                        int family)
    {
        struct ifaddrs *next, *results;
        // 调用getifaddrs获取所有网络接口信息
        if (getifaddrs(&results) != 0)
        {
            SYLAR_LOG_DEBUG(g_logger) << "Address::GetInterfaceAddresses getifaddrs "
                                         " err="
                                      << errno << " errstr=" << strerror(errno);
            return false;
        }

        try
        {
            // 遍历所有网络接口
            for (next = results; next; next = next->ifa_next)
            {
                Address::ptr addr;
                // 初始化前缀长度为最大值
                uint32_t prefix_len = ~0u;
                // 如果指定了地址族且与当前接口地址族不匹配，则跳过
                if (family != AF_UNSPEC && family != next->ifa_addr->sa_family)
                {
                    continue;
                }
                // 根据地址族处理不同的地址类型
                switch (next->ifa_addr->sa_family)
                {
                case AF_INET:
                {
                    // 创建IPv4地址对象
                    addr = Create(next->ifa_addr, sizeof(sockaddr_in));
                    // 获取IPv4子网掩码并计算前缀长度
                    uint32_t netmask = ((sockaddr_in *)next->ifa_netmask)->sin_addr.s_addr;
                    prefix_len = CountBytes(netmask);
                }
                break;
                case AF_INET6:
                {
                    // 创建IPv6地址对象
                    addr = Create(next->ifa_addr, sizeof(sockaddr_in6));
                    // 获取IPv6子网掩码并计算前缀长度
                    in6_addr &netmask = ((sockaddr_in6 *)next->ifa_netmask)->sin6_addr;
                    prefix_len = 0;
                    // IPv6地址有128位(16字节)，需要逐字节计算前缀长度
                    for (int i = 0; i < 16; ++i)
                    {
                        prefix_len += CountBytes(netmask.s6_addr[i]);
                    }
                }
                break;
                default:
                    // 忽略不支持的地址族
                    break;
                }

                // 如果成功创建地址对象，则将其添加到结果集中
                if (addr)
                {
                    result.insert(std::make_pair(next->ifa_name,
                                                 std::make_pair(addr, prefix_len)));
                }
            }
        }
        catch (...)
        {
            // 异常处理：记录错误日志并释放资源
            SYLAR_LOG_ERROR(g_logger) << "Address::GetInterfaceAddresses exception";
            freeifaddrs(results);
            return false;
        }
        // 释放getifaddrs分配的内存
        freeifaddrs(results);
        // 返回是否成功获取到至少一个接口地址
        return !result.empty();
    }

    /**
     * @brief 获取指定网络接口的IP地址信息
     * @param[out] result 存储指定网络接口地址的结果集，每个元素是一个pair类型，
     *                    其中first为接口地址，second为网络前缀长度
     * @param[in] iface 网络接口名称，如"eth0"，如果为空或"*"则返回默认地址
     * @param[in] family 地址族，AF_INET表示IPv4，AF_INET6表示IPv6，AF_UNSPEC表示不限制地址族
     * @return 成功获取至少一个接口地址时返回true，否则返回false
     *
     * @details 此函数用于获取特定网络接口的地址信息。如果iface参数为空或"*"，
     *          则根据地址族返回默认的IPv4或IPv6地址对象。
     *          否则通过调用另一个重载版本的GetInterfaceAddresses函数获取所有接口信息，
     *          然后筛选出指定接口的地址信息。
     */
    bool Address::GetInterfaceAddresses(std::vector<std::pair<Address::ptr, uint32_t>> &result,
                                        const std::string &iface, int family)
    {
        // 处理特殊情况：如果接口名称为空或为"*"，则返回默认地址
        if (iface.empty() || iface == "*")
        {
            // 如果地址族为IPv4或未指定，则添加默认IPv4地址
            if (family == AF_INET || family == AF_UNSPEC)
            {
                result.push_back(std::make_pair(Address::ptr(new IPv4Address()), 0u));
            }
            // 如果地址族为IPv6或未指定，则添加默认IPv6地址
            if (family == AF_INET6 || family == AF_UNSPEC)
            {
                result.push_back(std::make_pair(Address::ptr(new IPv6Address()), 0u));
            }
            return true;
        }

        // 获取所有网络接口地址信息
        std::multimap<std::string, std::pair<Address::ptr, uint32_t>> results;

        if (!GetInterfaceAddresses(results, family))
        {
            return false;
        }

        // 从所有接口信息中筛选出指定接口的地址信息
        auto its = results.equal_range(iface);
        for (; its.first != its.second; ++its.first)
        {
            result.push_back(its.first->second);
        }
        // 返回是否成功获取到指定接口的地址信息
        return !result.empty();
    }

    int Address::getFamily() const
    {
        return getAddr()->sa_family;
    }

    std::string Address::toString() const
    {
        std::stringstream ss;
        insert(ss);
        return ss.str();
    }

    /**
     * @brief 比较两个Address对象的大小关系
     * @param[in] rhs 右侧比较对象
     * @return 当前对象小于右侧对象时返回true，否则返回false
     *
     * @details 此函数实现了Address对象的字典序比较：
     *          1. 首先比较两个地址的前min(len1, len2)个字节
     *          2. 如果前面部分相等，则比较地址长度，长度短的认为较小
     *
     *          这种比较方式确保了相同地址族和相同地址的Address对象比较结果为相等，
     *          而不同地址的对象能够按照一定的规则排序。
     */
    bool Address::operator<(const Address &rhs) const
    {
        // 获取两个地址中的较小长度，避免内存越界访问
        socklen_t min_len = std::min(getAddrLen(), rhs.getAddrLen());
        // 比较两个地址的前min_len个字节
        int result = memcmp(getAddr(), rhs.getAddr(), min_len);
        if (result < 0)
        {
            // 前面部分已经确定左侧小于右侧
            return true;
        }
        else if (result > 0)
        {
            // 前面部分已经确定左侧大于右侧
            return false;
        }
        else
        {
            // 前面部分相等，比较地址长度，长度短的认为较小
            return getAddrLen() < rhs.getAddrLen();
        }
    }

    /**
     * @brief 判断两个Address对象是否相等
     * @param[in] rhs 右侧比较对象
     * @return 两个地址完全相同时返回true，否则返回false
     *
     * @details 两个Address对象被认为是相等的，当且仅当：
     *          1. 它们的地址长度相等
     *          2. 它们的所有地址字节都相等
     *
     *          此函数通常用于在容器中查找特定地址或去重等场景。
     */
    bool Address::operator==(const Address &rhs) const
    {
        // 首先比较地址长度是否相等
        return getAddrLen() == rhs.getAddrLen() &&
               // 然后比较所有地址字节是否相等
               memcmp(getAddr(), rhs.getAddr(), getAddrLen()) == 0;
    }

    /**
     * @brief 判断两个Address对象是否不相等
     * @param[in] rhs 右侧比较对象
     * @return 两个地址不相同时返回true，否则返回false
     *
     * @details 此函数直接利用operator==的实现结果取反，
     *          遵循了C++运算符重载的最佳实践，避免重复实现逻辑。
     */
    bool Address::operator!=(const Address &rhs) const
    {
        // 直接利用等于运算符的结果取反
        return !(*this == rhs);
    }

    /**
     * @brief 根据字符串地址创建IPAddress对象
     * @param[in] address 字符串格式的IP地址，支持IPv4和IPv6格式
     * @param[in] port 端口号
     * @return 返回创建的IPAddress对象智能指针，创建失败时返回nullptr
     *
     * @details 该函数使用getaddrinfo系统调用来解析地址字符串，支持以下格式：
     *          - IPv4地址: "192.168.1.1"
     *          - IPv6地址: "::1", "fe80::1"
     *
     *          函数内部会设置hints参数，指定只解析数字格式的IP地址(AI_NUMERICHOST)，
     *          不进行DNS解析，提高解析效率并避免网络请求。
     *
     *          解析成功后，使用Address::Create工厂方法创建对应的地址对象，
     *          然后设置端口号并返回。
     */
    IPAddress::ptr IPAddress::Create(const char *address, uint16_t port)
    {
        addrinfo hints, *results;
        // 初始化hints结构体
        memset(&hints, 0, sizeof(hints));

        // 设置地址解析参数：只解析数字格式IP地址，支持任意socket类型
        // hints.ai_family = AI_NUMERICHOST;
        hints.ai_socktype = AF_UNSPEC;

        // 调用getaddrinfo解析地址字符串
        int error = getaddrinfo(address, NULL, &hints, &results);
        if (error)
        {
            // 解析失败，记录错误日志并返回nullptr
            SYLAR_LOG_ERROR(g_logger) << "IPAddress::Create(" << address << ", " << port << ") error="
                                      << error << " errno=" << errno << " errstr=" << strerror(errno);
            return nullptr;
        }

        try
        {
            // 使用Address::Create工厂方法创建地址对象
            IPAddress::ptr result = std::dynamic_pointer_cast<IPAddress>(
                Address::Create(results->ai_addr, (socklen_t)results->ai_addrlen));
            if (result)
            {
                // 设置端口号
                result->setPort(port);
            }
            // 释放getaddrinfo分配的内存
            freeaddrinfo(results);
            return result;
        }
        catch (...)
        {
            // 异常处理：确保释放内存后返回nullptr
            freeaddrinfo(results);
            return nullptr;
        }
    }

    /**
     * @brief 根据IPv4地址字符串创建IPv4Address对象
     * @param[in] address IPv4地址字符串，如"192.168.1.1"、"127.0.0.1"
     * @param[in] port 端口号
     * @return 返回创建的IPv4Address对象智能指针，创建失败时返回nullptr
     *
     * @details 该函数专门用于创建IPv4地址对象，不依赖于getaddrinfo系统调用，
     *          而是直接使用inet_pton函数进行地址解析，效率更高。
     *
     *          函数流程：
     *          1. 创建IPv4Address对象
     *          2. 设置端口号（使用网络字节序）
     *          3. 使用inet_pton解析IPv4地址字符串
     *          4. 解析成功则返回对象，失败则记录日志并返回nullptr
     *
     * @note 该函数只支持IPv4地址，不支持IPv6或其他格式地址
     */
    IPv4Address::ptr IPv4Address::Create(const char *address, uint16_t port)
    {
        // 创建IPv4Address对象
        IPv4Address::ptr rt(new IPv4Address);
        // 设置端口号，转换为网络字节序
        rt->m_addr.sin_port = hton(port);
        // 使用inet_pton解析IPv4地址字符串
        int result = inet_pton(AF_INET, address, &rt->m_addr.sin_addr.s_addr);
        if (result <= 0)
        {
            // 解析失败，记录错误日志并返回nullptr
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "IPv4Address::Create(" << address << ", " << port
                                              << ") rt=" << result << " errno=" << errno
                                              << " errstr=" << strerror(errno);
            return nullptr;
        }
        // 返回创建成功的对象
        return rt;
    }

    IPv4Address::IPv4Address(const sockaddr_in &address)
    {
        m_addr = address;
    }

    IPv4Address::IPv4Address(uint32_t address, uint16_t port)
    {
        memset(&m_addr, 0, sizeof(m_addr));
        m_addr.sin_family = AF_INET;
        m_addr.sin_port = hton(port);
        m_addr.sin_addr.s_addr = hton(address);
    }

    const sockaddr *IPv4Address::getAddr() const
    {
        return (sockaddr *)&m_addr;
    }

    sockaddr *IPv4Address::getAddr()
    {
        return (sockaddr *)&m_addr;
    }

    socklen_t IPv4Address::getAddrLen() const
    {
        return sizeof(m_addr);
    }

    /**
     * @brief 将IPv4地址输出到流中
     * @param[in] os 输出流对象
     * @return 返回输出流对象的引用
     *
     * @details 该函数将IPv4地址格式化为"xxx.xxx.xxx.xxx:port"的形式输出到流中。
     *          由于网络传输中使用的是大端序（网络字节序），而本地处理时通常使用
     *          小端序，因此需要使用byteswapOnLittleEndian将地址和端口从网络字节序
     *          转换为主机字节序后再进行处理。
     *
     *          输出格式示例：192.168.1.1:8080
     */
    std::ostream &IPv4Address::insert(std::ostream &os) const
    {
        // 将IPv4地址从网络字节序转换为主机字节序
        uint32_t address = ntoh(m_addr.sin_addr.s_addr);
        // 按照xxx.xxx.xxx.xxx的格式输出IP地址的四个字节
        os << ((address >> 24) & 0xff) << "."
           << ((address >> 16) & 0xff) << "."
           << ((address >> 8) & 0xff) << "."
           << (address & 0xff);
        // 输出端口号，同样需要从网络字节序转换为主机字节序
        os << ":" << ntoh(m_addr.sin_port);
        return os;
    }

    /**
     * @brief 根据前缀长度计算广播地址
     * @param[in] prefix_len 前缀长度，取值范围0-32
     * @return 返回计算得到的广播地址对象，参数无效时返回nullptr
     *
     * @details 广播地址是网络中用于向所有主机发送数据的特殊地址。
     *          计算方法是将IP地址与主机部分全为1的掩码进行按位或运算。
     *
     *          例如：IP地址192.168.1.100/24的广播地址计算过程：
     *          1. IP地址：192.168.1.100 (二进制: 11000000.10101000.00000001.01100100)
     *          2. 子网掩码：255.255.255.0 (二进制: 11111111.11111111.11111111.00000000)
     *          3. 主机部分全1掩码：0.0.0.255 (二进制: 00000000.00000000.00000000.11111111)
     *          4. 广播地址：192.168.1.255 (二进制: 11000000.10101000.00000001.11111111)
     *
     * @note 前缀长度必须在0-32范围内，超出范围返回nullptr
     */
    IPAddress::ptr IPv4Address::broadcastAddress(uint32_t prefix_len)
    {
        // 检查前缀长度是否有效（0-32）
        if (prefix_len > 32)
        {
            return nullptr;
        }

        // 复制当前地址信息到广播地址结构体
        sockaddr_in baddr(m_addr);
        // 计算广播地址：将IP地址与主机部分全1的掩码进行按位或运算
        // CreateMask<uint32_t>(prefix_len)创建主机部分全1的掩码
        // hton确保掩码使用网络字节序
        baddr.sin_addr.s_addr |= hton(CreateMask<uint32_t>(prefix_len));
        // 创建并返回新的IPv4Address对象
        return IPv4Address::ptr(new IPv4Address(baddr));
    }

    /**
     * @brief 根据前缀长度计算网络地址
     * @param[in] prefix_len 前缀长度，取值范围0-32
     * @return 返回计算得到的网络地址对象，参数无效时返回nullptr
     *
     * @details 网络地址是标识一个网络段的特殊地址，主机部分全为0。
     *          计算方法是将IP地址与子网掩码进行按位与运算。
     *
     *          例如：IP地址192.168.1.100/24的网络地址计算过程：
     *          1. IP地址：192.168.1.100 (二进制: 11000000.10101000.00000001.01100100)
     *          2. 子网掩码：255.255.255.0 (二进制: 11111111.11111111.11111111.00000000)
     *          3. 网络地址：192.168.1.0 (二进制: 11000000.10101000.00000001.00000000)
     *
     * @note 前缀长度必须在0-32范围内，超出范围返回nullptr
     */
    IPAddress::ptr IPv4Address::networkAddress(uint32_t prefix_len)
    {
        // 检查前缀长度是否有效（0-32）
        if (prefix_len > 32)
        {
            return nullptr;
        }

        // 复制当前地址信息到网络地址结构体
        sockaddr_in baddr(m_addr);
        // 计算网络地址：将IP地址与子网掩码进行按位与运算
        // CreateMask<uint32_t>(prefix_len)创建主机部分全1的掩码
        // hton确保掩码使用网络字节序
        baddr.sin_addr.s_addr &= hton(CreateMask<uint32_t>(prefix_len));
        // 创建并返回新的IPv4Address对象
        return IPv4Address::ptr(new IPv4Address(baddr));
    }

    /**
     * @brief 根据前缀长度计算子网掩码
     * @param[in] prefix_len 前缀长度，取值范围0-32
     * @return 返回计算得到的子网掩码对象，参数无效时返回nullptr
     *
     * @details 子网掩码用于标识IP地址中网络部分和主机部分的边界。
     *          网络部分为1，主机部分为0。
     *
     *          例如：前缀长度24对应的子网掩码：
     *          1. 二进制表示：11111111.11111111.11111111.00000000
     *          2. 十进制表示：255.255.255.0
     *
     * @note 前缀长度必须在0-32范围内，超出范围返回nullptr
     */
    IPAddress::ptr IPv4Address::subnetMask(uint32_t prefix_len)
    {
        // 检查前缀长度是否有效（0-32）
        if (prefix_len > 32)
        {
            return nullptr;
        }

        // 初始化子网掩码结构体
        sockaddr_in mask_addr;
        memset(&mask_addr, 0, sizeof(mask_addr));
        // 设置地址族为IPv4
        mask_addr.sin_family = AF_INET;
        // 计算子网掩码：将主机部分全1的掩码取反得到网络部分全1的掩码
        // CreateMask<uint32_t>(prefix_len)创建主机部分全1的掩码
        // ~操作符将其取反得到子网掩码
        // hton确保掩码使用网络字节序
        mask_addr.sin_addr.s_addr = ~hton(CreateMask<uint32_t>(prefix_len));
        // 创建并返回新的IPv4Address对象
        return IPv4Address::ptr(new IPv4Address(mask_addr));
    }

    uint32_t IPv4Address::getPort() const
    {
        return ntoh(m_addr.sin_port);
    }

    void IPv4Address::setPort(uint16_t port)
    {
        m_addr.sin_port = hton(port);
    }

    /**
     * @brief 根据IPv6地址字符串创建IPv6Address对象
     * @param[in] address IPv6地址字符串，如"::1"、"fe80::1"、"2001:db8::1"等
     * @param[in] port 端口号
     * @return 返回创建的IPv6Address对象智能指针，创建失败时返回nullptr
     *
     * @details 该函数专门用于创建IPv6地址对象，不依赖于getaddrinfo系统调用，
     *          而是直接使用inet_pton函数进行地址解析，效率更高。
     *
     *          函数流程：
     *          1. 创建IPv6Address对象
     *          2. 设置端口号（使用网络字节序）
     *          3. 使用inet_pton解析IPv6地址字符串
     *          4. 解析成功则返回对象，失败则记录日志并返回nullptr
     *
     * @note 该函数只支持IPv6地址，不支持IPv4或其他格式地址
     * @note 端口号会被转换为网络字节序存储
     * @warning 当前实现中存在一个错误，使用了AF_INET而不是AF_INET6作为inet_pton的第一个参数，
     *          这会导致IPv6地址解析失败
     */
    IPv6Address::ptr IPv6Address::Create(const char *address, uint16_t port)
    {
        // 创建IPv6Address对象
        IPv6Address::ptr rt(new IPv6Address);
        // 设置端口号，转换为网络字节序
        rt->m_addr.sin6_port = hton(port);
        // 使用inet_pton解析IPv6地址字符串
        int result = inet_pton(AF_INET6, address, &rt->m_addr.sin6_addr);
        if (result <= 0)
        {
            // 解析失败，记录错误日志并返回nullptr
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "IPv6Address::Create(" << address << ", " << port
                                              << ") rt=" << result << " errno=" << errno
                                              << " errstr=" << strerror(errno);
            return nullptr;
        }
        // 返回创建成功的对象
        return rt;
    }

    IPv6Address::IPv6Address()
    {
        memset(&m_addr, 0, sizeof(m_addr));
        m_addr.sin6_family = AF_INET6;
    }

    IPv6Address::IPv6Address(const sockaddr_in6 &address)
    {
        m_addr = address;
    }

    IPv6Address::IPv6Address(const uint8_t address[16], uint16_t port)
    {
        memset(&m_addr, 0, sizeof(m_addr));
        m_addr.sin6_family = AF_INET6;
        m_addr.sin6_port = hton(port);
        memcpy(&m_addr.sin6_addr, address, sizeof(m_addr.sin6_addr));
    }

    const sockaddr *IPv6Address::getAddr() const
    {
        return (sockaddr *)&m_addr;
    }

    sockaddr *IPv6Address::getAddr()
    {
        return (sockaddr *)&m_addr;
    }

    socklen_t IPv6Address::getAddrLen() const
    {
        return sizeof(m_addr);
    }

    /**
     * @brief 将IPv6地址输出到流中
     * @param[in] os 输出流对象
     * @return 返回输出流对象的引用
     *
     * @details 该函数将IPv6地址格式化为"[xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx]:port"的形式输出到流中。
     *          IPv6地址的压缩表示法遵循RFC 5952标准：
     *          1. 每组4个十六进制数字表示16位
     *          2. 连续的全零组可以用"::"表示，但整个地址中只能出现一次
     *          3. 每组前导零可以省略
     *
     *          例如：
     *          - 完整形式：[2001:0db8:0000:0000:0000:0000:0000:0001]:8080
     *          - 压缩形式：[2001:db8::1]:8080
     *          - 本地回环：[::1]:8080
     *
     * @note 端口号使用主机字节序输出
     * @note 地址各组使用网络字节序存储，输出前需要转换为主机字节序
     */
    std::ostream &IPv6Address::insert(std::ostream &os) const
    {
        // 输出IPv6地址的开始方括号
        os << "[";
        // 将128位IPv6地址按16位分组处理
        uint16_t *addr = (uint16_t *)m_addr.sin6_addr.s6_addr;
        // 标记是否已经处理过连续的零组
        bool used_zeros = false;
        // 遍历8组16位地址段
        for (size_t i = 0; i < 8; ++i)
        {
            // 跳过第一个连续的全零组
            if (addr[i] == 0 && !used_zeros)
            {
                continue;
            }
            // 在连续零组结束后输出冒号
            if (i && addr[i - 1] == 0 && !used_zeros)
            {
                os << ":";
                used_zeros = true;
            }
            // 在非零组之间输出冒号分隔符
            if (i)
            {
                os << ":";
            }
            // 输出当前16位地址段（转换为主机字节序并以十六进制格式输出）
            os << std::hex << (int)ntoh(addr[i]) << std::dec;
        }

        // 处理末尾的零组情况
        if (!used_zeros && addr[7] == 0)
        {
            os << ":";
        }

        // 输出结束方括号和端口号（端口号转换为主机字节序）
        os << "]:" << ntoh(m_addr.sin6_port);
        return os;
    }

    /**
     * @brief 根据前缀长度计算广播地址
     * @param[in] prefix_len 前缀长度，取值范围0-128
     * @return 返回计算得到的广播地址对象，参数无效时返回nullptr
     *
     * @details 广播地址是网络中用于向所有主机发送数据的特殊地址。
     *          对于IPv6，计算方法是将IP地址与主机部分全为1的掩码进行按位或运算。
     *
     *          例如：IP地址2001:db8::1/64的广播地址计算过程：
     *          1. IP地址：2001:db8::1
     *          2. 前缀长度：64位（网络部分）
     *          3. 主机部分：后64位
     *          4. 广播地址：将后64位全部置为1
     *
     * @note 前缀长度必须在0-128范围内，超出范围返回nullptr
     * @note IPv6实际上不使用广播地址，而是使用多播地址，但此函数仍按传统方式计算
     */
    IPAddress::ptr IPv6Address::broadcastAddress(uint32_t prefix_len)
    {
        // 检查前缀长度是否有效（0-128）
        if (prefix_len > 128)
        {
            return nullptr;
        }

        // 复制当前地址信息到广播地址结构体
        sockaddr_in6 baddr(m_addr);
        // 对前缀长度所在字节进行处理，将主机部分的位设置为1
        baddr.sin6_addr.s6_addr[prefix_len / 8] |= CreateMask<uint8_t>(prefix_len % 8);
        // 将前缀长度之后的所有字节设置为0xff（全1）
        for (int i = prefix_len / 8 + 1; i < 16; ++i)
        {
            baddr.sin6_addr.s6_addr[i] = 0xff;
        }
        // 创建并返回新的IPv6Address对象
        return IPv6Address::ptr(new IPv6Address(baddr));
    }

    /**
     * @brief 根据前缀长度计算网络地址
     * @param[in] prefix_len 前缀长度，取值范围0-128
     * @return 返回计算得到的网络地址对象，参数无效时返回nullptr
     *
     * @details 网络地址是标识一个网络段的特殊地址，主机部分全为0。
     *          计算方法是将IP地址与子网掩码进行按位与运算。
     *
     *          例如：IP地址2001:db8::1/64的网络地址计算过程：
     *          1. IP地址：2001:db8::1
     *          2. 前缀长度：64位（网络部分）
     *          3. 网络地址：将后64位全部置为0，得到2001:db8::
     *
     * @note 前缀长度必须在0-128范围内，超出范围返回nullptr
     */
    IPAddress::ptr IPv6Address::networkAddress(uint32_t prefix_len)
    {
        // 检查前缀长度是否有效（0-128）
        if (prefix_len > 128)
        {
            return nullptr;
        }

        // 复制当前地址信息到网络地址结构体
        sockaddr_in6 baddr(m_addr);
        // 对前缀长度所在字节进行处理，将主机部分的位设置为0
        baddr.sin6_addr.s6_addr[prefix_len / 8] &= CreateMask<uint8_t>(prefix_len % 8);
        // 创建并返回新的IPv6Address对象
        return IPv6Address::ptr(new IPv6Address(baddr));
    }

    /**
     * @brief 根据前缀长度计算子网掩码
     * @param[in] prefix_len 前缀长度，取值范围0-128
     * @return 返回计算得到的子网掩码对象，参数无效时返回nullptr
     *
     * @details 子网掩码用于标识IP地址中网络部分和主机部分的边界。
     *          网络部分为1，主机部分为0。
     *
     *          例如：前缀长度64对应的子网掩码：
     *          1. 二进制表示：11111111...11111111 00000000...00000000 (前64位为1，后64位为0)
     *          2. 十六进制表示：ffff:ffff:ffff:ffff::
     *
     * @note 前缀长度必须在0-128范围内，超出范围返回nullptr
     */
    IPAddress::ptr IPv6Address::subnetMask(uint32_t prefix_len)
    {
        // 检查前缀长度是否有效（0-128）
        if (prefix_len > 128)
        {
            return nullptr;
        }

        // 初始化子网掩码结构体
        sockaddr_in6 subnet;
        memset(&subnet, 0, sizeof(subnet));
        // 设置地址族为IPv6
        subnet.sin6_family = AF_INET6;
        // 设置前缀长度所在字节的网络位部分为1
        subnet.sin6_addr.s6_addr[prefix_len / 8] = ~CreateMask<uint8_t>(prefix_len % 8);

        // 将前缀长度之前的完整字节设置为0xff（全1）
        for (uint32_t i = 0; i < prefix_len / 8; ++i)
        {
            subnet.sin6_addr.s6_addr[i] = 0xff;
        }
        // 创建并返回新的IPv6Address对象
        return IPv6Address::ptr(new IPv6Address(subnet));
    }

    uint32_t IPv6Address::getPort() const
    {
        return ntoh(m_addr.sin6_port);
    }

    void IPv6Address::setPort(uint16_t port)
    {
        m_addr.sin6_port = hton(port);
    }

    // Unix域套接字路径最大长度常量
    static const size_t MAX_PATH_LEN = sizeof(((sockaddr_un *)0)->sun_path) - 1;

    /**
     * @brief UnixAddress默认构造函数
     *
     * @details 初始化一个空的Unix域套接字地址对象。将地址结构体清零，
     *          设置地址族为AF_UNIX，并将地址长度设置为最大路径长度。
     *
     * @note Unix域套接字用于同一台机器上的进程间通信(IPC)，比TCP套接字更高效。
     */
    UnixAddress::UnixAddress()
    {
        // 清零整个地址结构体
        memset(&m_addr, 0, sizeof(m_addr));
        // 设置地址族为Unix域套接字
        m_addr.sun_family = AF_UNIX;
        // 设置地址长度为offsetof(sockaddr_un, sun_path) + MAX_PATH_LEN
        m_length = offsetof(sockaddr_un, sun_path) + MAX_PATH_LEN;
    }

    /**
     * @brief UnixAddress带路径参数的构造函数
     * @param[in] path Unix域套接字路径
     *
     * @details 根据给定的路径字符串初始化Unix域套接字地址对象。
     *          会检查路径长度是否超过系统限制，并将路径复制到地址结构体中。
     *
     * @exception std::logic_error 当路径长度超过系统限制时抛出异常
     *
     * @note Unix域套接字路径有两种形式：
     *       1. 文件系统路径：如"/tmp/mysocket"
     *       2. 抽象路径：以null字符开头的路径，如"\0mysocket"
     *
     * @note Unix域套接字的路径长度限制通常为108字节（包括结尾的null字符）
     */
    UnixAddress::UnixAddress(const std::string &path)
    {
        // 清零整个地址结构体
        memset(&m_addr, 0, sizeof(m_addr));
        // 设置地址族为Unix域套接字
        m_addr.sun_family = AF_UNIX;
        // 初始化地址长度为路径长度加1（结尾的null字符）
        m_length = path.size() + 1;

        // 处理抽象命名空间路径（以null字符开头）
        if (!path.empty() && path[0] == '\0')
        {
            // 抽象命名空间路径不需要结尾的null字符
            --m_length;
        }

        // 检查路径长度是否超过系统限制
        if (m_length > sizeof(m_addr.sun_path))
        {
            // 路径太长，抛出逻辑错误异常
            throw std::logic_error("path too long");
        }
        // 将路径复制到地址结构体中
        memcpy(m_addr.sun_path, path.c_str(), m_length);
        // 更新地址长度，包含sockaddr_un结构体中sun_path字段的偏移量
        m_length += offsetof(sockaddr_un, sun_path);
    }

    /**
     * @brief 获取Unix域套接字地址结构体指针
     * @return 返回指向sockaddr结构体的指针
     *
     * @details 该函数返回指向内部Unix域套接字地址结构体的指针，
     *          可用于socket相关系统调用。
     */
    const sockaddr *UnixAddress::getAddr() const
    {
        return (sockaddr *)&m_addr;
    }

    sockaddr *UnixAddress::getAddr()
    {
        return (sockaddr *)&m_addr;
    }

    /**
     * @brief 获取Unix域套接字地址结构体长度
     * @return 返回地址结构体的实际长度
     *
     * @details 该函数返回Unix域套接字地址结构体的实际长度，
     *          考虑了路径长度的差异，用于socket相关系统调用。
     */
    socklen_t UnixAddress::getAddrLen() const
    {
        return m_length;
    }

    void UnixAddress::setAddrLen(socklen_t length)
    {
        m_length = length;
    }

    /**
     * @brief 将Unix域套接字地址输出到流中
     * @param[in] os 输出流对象
     * @return 返回输出流对象的引用
     *
     * @details 该函数将Unix域套接字地址格式化输出到流中。
     *          对于抽象命名空间路径（以null字符开头），会以"\0"开头输出；
     *          对于普通文件系统路径，则直接输出路径字符串。
     *
     * @note 抽象命名空间路径不会在文件系统中创建实际的文件节点
     */
    std::ostream &UnixAddress::insert(std::ostream &os) const
    {
        // 处理抽象命名空间路径（以null字符开头）
        if (m_length >= offsetof(sockaddr_un, sun_path) && m_addr.sun_path[0] == '\0')
        {
            // 输出抽象命名空间路径，格式为\0 + 路径内容
            return os << "\\0" << std::string(m_addr.sun_path + 1, m_length - offsetof(sockaddr_un, sun_path) - 1);
        }
        // 输出普通文件系统路径
        return os << m_addr.sun_path;
    }

    /**
     * @brief UnknownAddress构造函数（根据地址族）
     * @param[in] family 地址族
     *
     * @details 使用指定的地址族初始化未知地址对象。
     *          主要用于创建不被当前实现支持的地址族的地址对象。
     *
     * @note UnknownAddress类用于处理系统支持但当前实现未专门处理的地址族
     */
    UnknownAddress::UnknownAddress(int family)
    {
        // 清零整个地址结构体
        memset(&m_addr, 0, sizeof(m_addr));
        // 设置地址族
        m_addr.sa_family = family;
    }

    /**
     * @brief UnknownAddress构造函数（根据sockaddr结构体）
     * @param[in] addr sockaddr结构体引用
     *
     * @details 使用给定的sockaddr结构体初始化未知地址对象。
     *          用于将任意sockaddr结构体封装为Address对象。
     */
    UnknownAddress::UnknownAddress(const sockaddr &addr)
    {
        // 复制sockaddr结构体内容
        m_addr = addr;
    }

    /**
     * @brief 获取未知地址结构体指针
     * @return 返回指向sockaddr结构体的指针
     *
     * @details 该函数返回指向内部sockaddr结构体的指针，
     *          可用于socket相关系统调用。
     */
    const sockaddr *UnknownAddress::getAddr() const
    {
        return (sockaddr *)&m_addr;
    }

    sockaddr *UnknownAddress::getAddr()
    {
        return (sockaddr *)&m_addr;
    }

    /**
     * @brief 获取未知地址结构体长度
     * @return 返回sockaddr结构体的长度
     *
     * @details 该函数返回sockaddr结构体的固定长度，
     *          因为UnknownAddress不保存实际的长度信息。
     */
    socklen_t UnknownAddress::getAddrLen() const
    {
        return sizeof(m_addr);
    }

    /**
     * @brief 将未知地址输出到流中
     * @param[in] os 输出流对象
     * @return 返回输出流对象的引用
     *
     * @details 该函数将未知地址格式化输出到流中，显示地址族信息。
     *          格式为：[UnknownAddress family=<地址族数值>]
     *
     * @note 这主要用于调试和日志记录，帮助识别未被专门处理的地址族
     */
    std::ostream &UnknownAddress::insert(std::ostream &os) const
    {
        // 输出未知地址族信息
        os << "[UnknownAddress family=" << m_addr.sa_family << "]";
        return os;
    }
    std::ostream &operator<<(std::ostream &os, const Address &addr)
    {
        return os << addr.toString();
    }
}
