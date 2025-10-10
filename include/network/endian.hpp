/**
 * @file endian.hpp
 * @brief 字节序处理工具
 *
 * 该文件提供了一套跨平台的字节序处理机制，用于在不同字节序的系统间进行数据转换。
 * 主要功能包括：
 * 1. 基础字节交换函数：根据数据类型大小自动选择合适的字节交换操作
 * 2. 智能字节序转换函数：根据当前系统字节序自动决定是否需要进行字节交换
 * 3. 网络字节序转换支持：提供转换到大端序和小端序的便捷函数
 *
 * 在网络编程中，不同架构的机器可能使用不同的字节序：
 * - 大端序（Big Endian）：高位字节存储在低地址，网络协议标准
 * - 小端序（Little Endian）：低位字节存储在低地址，常见于x86等架构
 *
 * 本文件提供的函数能够自动适应不同平台，确保数据在网络传输中的一致性。
 */

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
    // 如果是大端系统，小端字节序转换函数调用 byteswap
    template <typename T>
    T byteswapOnLittleEndian(T n)
    {
        return byteswap(n);
    }

    // 如果是大端系统，大端字节序转换函数直接返回原值
    template <typename T>
    T byteswapOnBigEndian(T n)
    {
        return n;
    }

#else
    // 如果是小端系统，小端字节序转换函数直接返回原值
    template <typename T>
    T byteswapOnLittleEndian(T n)
    {
        return n;
    }

    // 如果是小端系统，大端字节序转换函数调用 byteswap
    template <typename T>
    T byteswapOnBigEndian(T n)
    {
        return byteswap(n);
    }
#endif
}