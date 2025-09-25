#include "util.hpp"
#include "macro.hpp"
#include "coroutine.hpp"
#include <execinfo.h>

namespace sylar
{
    static auto g_logger = SYLAR_LOG_NAME("system");

    pid_t GetThreadId()
    {
        return syscall(SYS_gettid);
    }

    uint64_t GetCoroutineId()
    {
        return Coroutine::GetCoroutineId();
    }

    /**
     * @brief 获取函数调用堆栈信息
     * @param bt[out] 存储堆栈信息的字符串向量
     * @param size[in] 需要获取的堆栈层数
     * @param skip[in] 跳过的堆栈层数，从第skip层开始记录
     *
     * 1、使用backtrace函数获取函数调用堆栈地址
     * 2、使用backtrace_symbols将地址转换为可读的符号字符串
     * 3、将符号信息存入传入的vector中，跳过指定层数(skip)
     * 4、释放分配的内存资源
     * 5、skip默认值为1，表示忽略掉Backtrace函数
     */
    void Backtrace(std::vector<std::string> &bt, int size, int skip)
    {
        // 分配存储堆栈地址的数组空间
        void **array = (void **)malloc(sizeof(void *) * size);
        // 获取函数调用堆栈地址
        size_t s = ::backtrace(array, size);

        // 将地址转换为可读的符号字符串
        char **strings = backtrace_symbols(array, s);
        if (NULL == strings)
        {
            SYLAR_LOG_ERROR(g_logger) << "backtrace_symbols error";
            free(array);
            return;
        }

        // 将堆栈符号信息存储到输出向量中
        for (size_t i = skip; i < s; ++i)
        {
            bt.push_back(strings[i]);
        }

        // 释放分配的内存
        free(array);
        free(strings);
    }

    /**
     * @brief 将函数调用堆栈信息转换为字符串
     * @param size 需要获取的堆栈层数
     * @param skip 跳过的堆栈层数，从第skip层开始记录
     * @param prefix 每行输出的前缀字符串
     * @return 包含堆栈信息的字符串
     *
     * 1、调用Backtrace函数获取调用栈信息，存储在vector中
     * 2、使用stringstream将每个栈帧信息加上前缀后拼接成一个字符串
     * 3、返回拼接后的完整调用栈字符串
     * 4、skip默认值为2，表示忽略掉BacktraceToString和Backtrace函数
     */
    std::string BacktraceToString(int size, int skip, const std::string &prefix)
    {
        std::vector<std::string> bt;
        Backtrace(bt, size, skip);
        std::stringstream ss;
        for (size_t i = 0; i < bt.size(); ++i)
        {
            ss << prefix << bt[i] << std::endl;
        }

        return ss.str();
    }
}