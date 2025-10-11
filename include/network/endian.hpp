#pragma once

#include <type_traits>
#include <cstdint>
#include <byteswap.h>

#define SYLAR_LITTLE_ENDIAN 1 // 定义小端字节序标识符
#define SYLAR_BIG_ENDIAN 2    // 定义大端字节序标识符

namespace sylar
{
    // 根据类型大小进行字节序转换的模板函数，使用SFINAE技术选择合适的实现
    template <typename T>
    typename std::enable_if<sizeof(T) == sizeof(uint64_t), T>::type
    byteswap(T value)
    {
        return (T)bswap_64((uint64_t)value); // 64位字节序转换
    }

    template <typename T>
    typename std::enable_if<sizeof(T) == sizeof(uint32_t), T>::type
    byteswap(T value)
    {
        return (T)bswap_32((uint32_t)value); // 32位字节序转换
    }

    template <typename T>
    typename std::enable_if<sizeof(T) == sizeof(uint16_t), T>::type
    byteswap(T value)
    {
        return (T)bswap_16((uint16_t)value); // 16位字节序转换
    }

    // 根据系统字节序定义 SYLAR_BYTE_ORDER
#if BYTE_ORDER == BIG_ENDIAN
#define SYLAR_BYTE_ORDER SYLAR_BIG_ENDIAN
#else
#define SYLAR_BYTE_ORDER SYLAR_LITTLE_ENDIAN
#endif

    // 针对不同系统字节序的字节序转换函数
#if SYLAR_BYTE_ORDER == SYLAR_BIG_ENDIAN
    // 在大端系统上进行字节转换
    template <typename T>
    T ntoh(T n)
    {
        return n;
    }

    template <typename T>
    T hton(T n)
    {
        return n;
    }

#else
    // 在小端系统上进行字节转换
    template <typename T>
    T ntoh(T n)
    {
        return byteswap(n);
    }

    template <typename T>
    T hton(T n)
    {
        return byteswap(n);
    }
#endif
}