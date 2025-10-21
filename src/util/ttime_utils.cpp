#include "ttime_utils.hpp"
#include <sys/time.h>
#include <ctime>

namespace sylar
{
    uint64_t TTime::NowToMS()
    {
        struct timeval tv;
        gettimeofday(&tv, nullptr);
        return tv.tv_sec * 1000 + tv.tv_usec / 1000;
    }

    uint64_t TTime::NowToUS()
    {
        struct timeval tv;
        gettimeofday(&tv, nullptr);
        return tv.tv_sec * 1000000 + tv.tv_usec;
    }

    uint64_t TTime::NowToS()
    {
        struct timeval tv;
        gettimeofday(&tv, nullptr); // 获取当前时间
        return tv.tv_sec;           // 返回秒数
    }

    uint64_t TTime::Now(int &year, int &month, int &day, int &hour, int &minute, int &second)
    {
        struct tm tm;
        time_t t = time(nullptr); // 获取当前时间戳（Unix 时间戳）
        localtime_r(&t, &tm);     // 线程安全地将时间戳转换为本地时间

        year = tm.tm_year + 1900;
        month = tm.tm_mon + 1;
        day = tm.tm_mday;
        hour = tm.tm_hour;
        minute = tm.tm_min;
        second = tm.tm_sec;
        return t; // 返回当前时间戳（秒）
    }

    std::string TTime::NowToString()
    {
        struct tm tm;             // 用于存储分解的时间值
        time_t t = time(nullptr); // 获取当前时间戳（Unix 时间戳）
        localtime_r(&t, &tm);     // 线程安全地将时间戳转换为本地时间

        char buf[128] = {0};
        size_t n = strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &tm);
        return std::string(buf, n); // 使用buf的前n个字符构造string对象
    }
}