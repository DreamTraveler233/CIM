#pragma once

#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <stdint.h>
#include <vector>
#include <string>
#include <fstream>
#include <cxxabi.h>

namespace sylar
{
    pid_t GetThreadId();

    uint64_t GetCoroutineId();

    /**
     * @brief 获取函数调用堆栈信息
     * @param bt[out] 存储堆栈信息的字符串向量
     * @param size[in] 需要获取的堆栈层数
     * @param skip[in] 跳过的堆栈层数，从第skip层开始记录
     */
    void Backtrace(std::vector<std::string> &bt, int size = 64, int skip = 1);

    /**
     * @brief 将函数调用堆栈信息转换为字符串
     * @param size 需要获取的堆栈层数
     * @param skip 跳过的堆栈层数，从第skip层开始记录
     * @param prefix 每行输出的前缀字符串
     * @return 包含堆栈信息的字符串
     */
    std::string BacktraceToString(int size = 64, int skip = 2, const std::string &prefix = "    ");

    

    

    template <class T>
    const char *TypeToName()
    {
        static const char *s_name = abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, nullptr);
        return s_name;
    }

    class FSUtil
    {
    public:
        static void ListAllFile(std::vector<std::string> &files, const std::string &path, const std::string &subfix);
        static bool Mkdir(const std::string &dirname);
        static bool IsRunningPidfile(const std::string &pidfile);
        static bool Rm(const std::string &path);
        static bool Mv(const std::string &from, const std::string &to);
        static bool Realpath(const std::string &path, std::string &rpath);
        static bool Symlink(const std::string &frm, const std::string &to);
        static bool Unlink(const std::string &filename, bool exist = false);
        static std::string Dirname(const std::string &filename);
        static std::string Basename(const std::string &filename);
        static bool OpenForRead(std::ifstream &ifs, const std::string &filename, std::ios_base::openmode mode);
        static bool OpenForWrite(std::ofstream &ofs, const std::string &filename, std::ios_base::openmode mode);
    };
}