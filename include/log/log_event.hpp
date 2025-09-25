#pragma once

#include "log_level.hpp"
#include <memory>
#include <sstream>

namespace sylar
{
    class Logger;

    class LogEvent
    {
    public:
        using ptr = std::shared_ptr<LogEvent>;

        LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level,
                 const char *file_name, int32_t line, uint32_t elapse,
                 uint32_t thread_id, uint32_t fiber_id, uint64_t time,
                 const std::string &thread_name);

        const char *getFileName() const;
        std::string getRelativeFileName() const;
        int32_t getLine() const;
        uint32_t getElapse() const;
        uint32_t getThreadId() const;
        const std::string &getThreadName() const;
        uint32_t getFiberId() const;
        uint64_t getTime() const;
        std::string getMessage() const;
        std::stringstream &getSS();
        std::shared_ptr<Logger> getLogger() const;
        LogLevel::Level getLevel() const;

        void format(const char *fmt, ...);
        void format(const char *fmt, va_list al);

    private:
        const char *m_fileName = nullptr;
        int32_t m_line = 0;
        uint32_t m_elapse = 0;
        uint32_t m_threadId = 0;
        std::string m_threadName;
        uint32_t m_fiberId = 0;
        uint64_t m_time;
        std::stringstream m_messageSS;
        LogLevel::Level m_level;
        std::shared_ptr<Logger> m_logger;
    };

    class LogEventWrap
    {
    public:
        LogEventWrap(LogEvent::ptr event);
        ~LogEventWrap();

        LogEvent::ptr getEvent() const;
        std::stringstream &getSS();

    private:
        LogEvent::ptr m_event;
    };

}