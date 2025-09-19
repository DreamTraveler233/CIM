#include "log_level.hpp"

namespace sylar
{
    const char *LogLevel::ToString(LogLevel::Level level)
    {
        switch (level)
        {
// 宏定义
#define XX(name)                \
    case LogLevel::Level::name: \
        return #name;

            XX(DEBUG);
            XX(INFO);
            XX(WARN);
            XX(ERROR);
            XX(FATAL);
// 取消宏定义
#undef XX
        default:
            return "UNKNOW";
        }
        return "UNKNOW";
    }

    LogLevel::Level LogLevel::FromString(const std::string &str)
    {
#define XX(level, name) \
    if (str == #name)   \
        return LogLevel::Level::level;

        XX(DEBUG, debug);
        XX(INFO, info);
        XX(WARN, warn);
        XX(ERROR, error);
        XX(FATAL, fatal);
        XX(DEBUG, DEBUG);
        XX(INFO, INFO);
        XX(WARN, WARN);
        XX(ERROR, ERROR);
        XX(FATAL, FATAL);
#undef XX
        return LogLevel::Level::UNKNOW;
    }
}