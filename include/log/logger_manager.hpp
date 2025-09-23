#pragma once

#include "singleton.hpp"
#include "log_level.hpp"
#include "lock.hpp"
#include <map>
#include <string>
#include <memory>
#include <vector>

namespace sylar
{
    class Logger;

    class LoggerManager
    {
    public:
        using MutexType = NullRWMutex;

        LoggerManager();
        std::shared_ptr<Logger> getLogger(const std::string &name);
        std::shared_ptr<Logger> getRoot() const;
        std::string toYamlString();

    private:
        std::map<std::string, std::shared_ptr<Logger>> m_loggers;
        std::shared_ptr<Logger> m_root;
        Mutex m_mutex;
        MutexType m_rwMutex;
    };

    using loggerMgr = sylar::Singleton<LoggerManager>;

    // 日志配置项
    struct LogAppenderDefine
    {
        int type = 0; // 1: FileLogAppender, 2: StdoutLogAppender
        LogLevel::Level level = LogLevel::Level::UNKNOWN;
        std::string formatter;
        std::string file;

        bool operator==(const LogAppenderDefine &other) const
        {
            return type == other.type &&
                   level == other.level &&
                   formatter == other.formatter &&
                   file == other.file;
        }
    };

    struct LogDefine
    {
        std::string name;
        LogLevel::Level level = LogLevel::Level::UNKNOWN;
        std::string formatter;
        std::vector<LogAppenderDefine> appenders;

        bool operator==(const LogDefine &other) const
        {
            return name == other.name &&
                   level == other.level &&
                   formatter == other.formatter &&
                   appenders == other.appenders;
        }

        bool operator<(const LogDefine &other) const
        {
            return name < other.name;
        }
    };
}