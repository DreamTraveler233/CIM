#pragma once

#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <stdint.h>
#include <vector>
#include <string>

namespace sylar
{
    pid_t GetThreadId();

    uint32_t GetFiberId();

    /**
     * @brief 获取函数调用堆栈信息
     * @param bt[out] 存储堆栈信息的字符串向量
     * @param size[in] 需要获取的堆栈层数
     * @param skip[in] 跳过的堆栈层数，从第skip层开始记录
     */
    void Backtrace(std::vector<std::string> &bt, int size, int skip = 1);

    /**
     * @brief 将函数调用堆栈信息转换为字符串
     * @param size 需要获取的堆栈层数
     * @param skip 跳过的堆栈层数，从第skip层开始记录
     * @param prefix 每行输出的前缀字符串
     * @return 包含堆栈信息的字符串
     */
    std::string BacktraceToString(int size, int skip = 2, const std::string &prefix = "    ");
}