#pragma once

#include "log_level.hpp"
#include "log_event.hpp"
#include "log_appender.hpp"
#include "log_formatter.hpp"
#include "log.hpp"
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
        std::string m_name;
        LogLevel::Level m_level;
        std::list<LogAppender::ptr> m_appenders;
        LogFormatter::ptr m_formatter;
        Logger::ptr m_root;
    };

}