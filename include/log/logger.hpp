#pragma once

#include "log_level.hpp"
#include "log_event.hpp"
#include "log_appender.hpp"
#include "log_formatter.hpp"
#include "macro.hpp"
#include "lock.hpp"
#include <string>
#include <list>
#include <memory>

namespace sylar
{

    class LoggerManager;

    class Logger
    {
        friend class LoggerManager;

    public:
        using ptr = std::shared_ptr<Logger>;
        using MutexType = Mutex;

        Logger(const std::string &name = "root");

        void log(LogLevel::Level level, LogEvent::ptr event);
        void debug(LogEvent::ptr event);
        void info(LogEvent::ptr event);
        void warn(LogEvent::ptr event);
        void error(LogEvent::ptr event);
        void fatal(LogEvent::ptr event);

        void addAppender(LogAppender::ptr appender);
        void delAppender(LogAppender::ptr appender);
        void clearAppender();

        LogLevel::Level getLevel() const;
        void setLevel(LogLevel::Level level);
        const std::string &getName() const;
        void setFormatter(LogFormatter::ptr val);
        void setFormatter(const std::string &val);
        LogFormatter::ptr getFormatter() const;
        Logger::ptr getRoot() const;

        std::string toYamlString();

    private:
        std::string m_name;                      // 日志器名称
        LogLevel::Level m_level;                 // 日志级别
        std::list<LogAppender::ptr> m_appenders; // 日志附加器列表
        LogFormatter::ptr m_formatter;           // 日志格式器
        Logger::ptr m_root;                      // 根日志器
        MutexType m_mutex;                           // 互斥锁
    };
}