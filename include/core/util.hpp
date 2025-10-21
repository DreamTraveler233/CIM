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

    class TTime
    {
    public:
        /**
         * @brief 获取当前 UTC 时间（毫秒级）。
         *
         * @return uint64_t 当前 UTC 时间，单位为毫秒
         */
        static uint64_t NowToMS();

        /**
         * @brief 获取当前 UTC 时间（微秒级）。
         *
         * @return uint64_t 当前 UTC 时间，单位为微秒
         */
        static uint64_t NowToUS();

        /**
         * @brief 获取当前 UTC 时间（秒级）。
         *
         * @return uint64_t 当前 UTC 时间，单位为秒
         */
        static uint64_t NowToS();

        /**
         * @brief 获取当前本地时间的年月日时分秒，并返回当前时间戳（秒）。
         *
         * @param[out] year   年
         * @param[out] month  月
         * @param[out] day    日
         * @param[out] hour   时
         * @param[out] minute 分
         * @param[out] second 秒
         * @return uint64_t 当前时间戳（秒）
         */
        static uint64_t Now(int& year, int& month, int& day, int& hour, int& minute, int& second);

        /**
         * @brief 获取当前时间的 ISO 格式字符串（yyyy-MM-ddTHH:mm:ss）。
         *
         * @return std::string 当前时间的 ISO 格式字符串
         */
        static std::string NowToString();
    };

    class StringUtils
    {
    public:
        /**
         * @brief 判断字符串 str 是否以子串 sub 开头。
         * @param str 待判断的字符串
         * @param sub 前缀子串
         * @return true 如果 str 以 sub 开头
         * @return false 否则
         */
        static bool StartsWith(const std::string &str, const std::string &sub);

        /**
         * @brief 判断字符串 str 是否以子串 sub 结尾。
         * @param str 待判断的字符串
         * @param sub 后缀子串
         * @return true 如果 str 以 sub 结尾
         * @return false 否则
         */
        static bool EndsWith(const std::string &str, const std::string &sub);

        /**
         * @brief 获取文件路径字符串中的目录部分。
         *
         * @param path 文件路径
         * @return string 目录部分（不包含文件名），若无目录则返回"./"
         *
         * @par 示例
         * - 输入：/home/user/file.txt，返回：/home/user
         * - 输入：C:\data\test.txt，返回：C:\data
         * - 输入：file.txt，返回：./
         */
        static std::string FilePath(const std::string &path);

        /**
         * @brief 获取文件路径中的文件名和扩展名部分（即去掉目录，只保留最后的文件名部分）。
         *
         * @param path 文件路径
         * @return string 文件名及扩展名
         *
         * @par 示例
         * - 输入：/home/user/file.txt，返回：file.txt
         * - 输入：C:\data\test.log，返回：test.log
         * - 输入：file.txt，返回：file.txt
         * - 输入：/home/user/，返回：/home/user/
         */
        static std::string FileNameExt(const std::string &path);

        /**
         * @brief 获取文件路径中的文件名（不带扩展名）。
         *
         * @param path 文件路径
         * @return string 文件名（不含扩展名）
         *
         * @par 示例
         * - 输入：/home/user/file.txt，返回：file
         * - 输入：C:\data\test.log，返回：test
         * - 输入：file，返回：file
         * - 输入：.bashrc，返回：.bashrc
         */
        static std::string FileName(const std::string &path);

        /**
         * @brief 获取文件路径中的扩展名部分（包括点）。
         *
         * @param path 文件路径
         * @return string 扩展名（包含点），若无扩展名返回空字符串
         *
         * @par 示例
         * - 输入：/home/user/file.txt，返回：.txt
         * - 输入：C:\data\test.log，返回：.log
         * - 输入：file，返回：""
         * - 输入：.bashrc，返回：""
         */
        static std::string Extension(const std::string &path);

        /**
         * @brief 将一个字符串按指定分隔符切分成多个子串。
         *
         * @param str 待分割的字符串
         * @param delimiter 分隔符
         * @return vector<string> 分割后的子串数组
         *
         * @par 示例
         * - 输入：str = "a,b,c", delimiter = ","，返回：["a", "b", "c"]
         * - 输入：str = "a--b--c", delimiter = "--"，返回：["a", "b", "c"]
         * - 输入：str = "abc", delimiter = ","，返回：["abc"]
         * - 输入：str = ",a,b,", delimiter = ","，返回：["a", "b"]
         */
        static std::vector<std::string> SplitString(const std::string &str, const std::string &delimiter);
    };

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