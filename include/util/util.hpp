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
#include <yaml-cpp/yaml.h>
#include <jsoncpp/json/json.h>
#include <cxxabi.h>
#include <boost/lexical_cast.hpp>

namespace CIM
{
    /**
     * @brief 获取当前线程的真实线程ID
     * @return 当前线程的系统线程ID（TID）
     *
     * 该函数通过系统调用获取当前线程的真实线程ID，
     * 与pthread_self()返回的pthread_t不同，该ID是系统级别的线程标识符。
     * 主要用于日志记录、调试和线程识别等场景。
     */
    pid_t GetThreadId();

    /**
     * @brief 获取当前协程ID
     * @return 当前协程的唯一标识符
     *
     * 该函数返回当前正在运行的协程ID。
     * 在协程编程中用于识别和跟踪不同的协程执行单元。
     * 如果当前不在协程环境中，可能返回特定的默认值（如0表示主线程）。
     */
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

    template <class V, class Map, class K>
    V GetParamValue(const Map &m, const K &k, const V &def = V())
    {
        auto it = m.find(k);
        if (it == m.end())
        {
            return def;
        }
        try
        {
            return boost::lexical_cast<V>(it->second);
        }
        catch (...)
        {
        }
        return def;
    }

    template <class V, class Map, class K>
    bool CheckGetParamValue(const Map &m, const K &k, V &v)
    {
        auto it = m.find(k);
        if (it == m.end())
        {
            return false;
        }
        try
        {
            v = boost::lexical_cast<V>(it->second);
            return true;
        }
        catch (...)
        {
        }
        return false;
    }

    bool YamlToJson(const YAML::Node &ynode, Json::Value &jnode);
    bool JsonToYaml(const Json::Value &jnode, YAML::Node &ynode);

    std::string GetHostName();
    std::string GetIPv4();

    template <class T>
    void nop(T *) {}

    template <class T>
    void delete_array(T *v)
    {
        if (v)
        {
            delete[] v;
        }
    }

    class TypeUtil
    {
    public:
        static int8_t ToChar(const std::string &str);
        static int64_t Atoi(const std::string &str);
        static double Atof(const std::string &str);
        static int8_t ToChar(const char *str);
        static int64_t Atoi(const char *str);
        static double Atof(const char *str);
    };

    class Atomic
    {
    public:
        template <class T, class S = T>
        static T addFetch(volatile T &t, S v = 1)
        {
            return __sync_add_and_fetch(&t, (T)v);
        }

        template <class T, class S = T>
        static T subFetch(volatile T &t, S v = 1)
        {
            return __sync_sub_and_fetch(&t, (T)v);
        }

        template <class T, class S>
        static T orFetch(volatile T &t, S v)
        {
            return __sync_or_and_fetch(&t, (T)v);
        }

        template <class T, class S>
        static T andFetch(volatile T &t, S v)
        {
            return __sync_and_and_fetch(&t, (T)v);
        }

        template <class T, class S>
        static T xorFetch(volatile T &t, S v)
        {
            return __sync_xor_and_fetch(&t, (T)v);
        }

        template <class T, class S>
        static T nandFetch(volatile T &t, S v)
        {
            return __sync_nand_and_fetch(&t, (T)v);
        }

        template <class T, class S>
        static T fetchAdd(volatile T &t, S v = 1)
        {
            return __sync_fetch_and_add(&t, (T)v);
        }

        template <class T, class S>
        static T fetchSub(volatile T &t, S v = 1)
        {
            return __sync_fetch_and_sub(&t, (T)v);
        }

        template <class T, class S>
        static T fetchOr(volatile T &t, S v)
        {
            return __sync_fetch_and_or(&t, (T)v);
        }

        template <class T, class S>
        static T fetchAnd(volatile T &t, S v)
        {
            return __sync_fetch_and_and(&t, (T)v);
        }

        template <class T, class S>
        static T fetchXor(volatile T &t, S v)
        {
            return __sync_fetch_and_xor(&t, (T)v);
        }

        template <class T, class S>
        static T fetchNand(volatile T &t, S v)
        {
            return __sync_fetch_and_nand(&t, (T)v);
        }

        template <class T, class S>
        static T compareAndSwap(volatile T &t, S old_val, S new_val)
        {
            return __sync_val_compare_and_swap(&t, (T)old_val, (T)new_val);
        }

        template <class T, class S>
        static bool compareAndSwapBool(volatile T &t, S old_val, S new_val)
        {
            return __sync_bool_compare_and_swap(&t, (T)old_val, (T)new_val);
        }
    };
}