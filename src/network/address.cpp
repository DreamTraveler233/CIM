#include "address.hpp"
#include <sstream>
#include <cstring>
#include <algorithm>

namespace sylar
{
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

    bool Address::operator<(const Address &rhs) const
    {
        socklen_t min_len = std::min(getAddrLen(), rhs.getAddrLen());
        int result = memcmp(getAddr(), rhs.getAddr(), min_len);
        if (result < 0)
        {
            return true;
        }
        else if (result > 0)
        {
            return false;
        }
        else
        {
            return getAddrLen() < rhs.getAddrLen();
        }
    }

    bool Address::operator==(const Address &rhs) const
    {
        return getAddrLen() == rhs.getAddrLen() &&
               memcmp(getAddr(), rhs.getAddr(), getAddrLen()) == 0;
    }

    bool Address::operator!=(const Address &rhs) const
    {
        return !(*this == rhs);
    }

    IPv4Address::IPv4Address(uint32_t address, uint32_t port)
    {
    }

    const sockaddr *IPv4Address::getAddr() const
    {
        return nullptr;
    }

    socklen_t IPv4Address::getAddrLen() const
    {
        return socklen_t();
    }
    
    std::ostream &IPv4Address::insert(std::ostream &os) const
    {
        // TODO: 在此处插入 return 语句
    }

    IPAddress::ptr IPv4Address::broadcastAddress(uint32_t prefix_len)
    {
        return IPAddress::ptr();
    }

    IPAddress::ptr IPv4Address::networdAddress(uint32_t prefix_len)
    {
        return IPAddress::ptr();
    }

    IPAddress::ptr IPv4Address::subnetMask(uint32_t prefix_len)
    {
        return IPAddress::ptr();
    }

    uint32_t IPv4Address::getPort() const
    {
        return 0;
    }

    void IPv4Address::setPort(uint32_t port)
    {
    }

    IPv6Address::IPv6Address(uint32_t address, uint32_t port)
    {
    }

    const sockaddr *IPv6Address::getAddr() const
    {
        return nullptr;
    }

    socklen_t IPv6Address::getAddrLen() const
    {
        return socklen_t();
    }

    std::ostream &IPv6Address::insert(std::ostream &os) const
    {
        // TODO: 在此处插入 return 语句
    }

    IPAddress::ptr IPv6Address::broadcastAddress(uint32_t prefix_len)
    {
        return IPAddress::ptr();
    }

    IPAddress::ptr IPv6Address::networdAddress(uint32_t prefix_len)
    {
        return IPAddress::ptr();
    }

    IPAddress::ptr IPv6Address::subnetMask(uint32_t prefix_len)
    {
        return IPAddress::ptr();
    }

    uint32_t IPv6Address::getPort() const
    {
        return 0;
    }

    void IPv6Address::setPort(uint32_t port)
    {
    }

    UnixAddress::UnixAddress(const std::string &path)
    {
    }

    const sockaddr *UnixAddress::getAddr() const
    {
        return nullptr;
    }

    socklen_t UnixAddress::getAddrLen() const
    {
        return socklen_t();
    }
    
    std::ostream &UnixAddress::insert(std::ostream &os) const
    {
        // TODO: 在此处插入 return 语句
    }

    const sockaddr *UnknownAddress::getAddr() const
    {
        return nullptr;
    }

    socklen_t UnknownAddress::getAddrLen() const
    {
        return socklen_t();
    }

    std::ostream &UnknownAddress::insert(std::ostream &os) const
    {
        // TODO: 在此处插入 return 语句
    }
}
