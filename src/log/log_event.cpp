#include "log_event.hpp"
#include "logger.hpp"
#include <cstdarg>

namespace sylar
{
    LogEvent::LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level,
                       const char *file_name, int32_t line, uint32_t elapse,
                       uint32_t thread_id, uint32_t fiber_id, uint64_t time)
        : m_fileName(file_name),
          m_line(line),
          m_elapse(elapse),
          m_threadId(thread_id),
          m_fiberId(fiber_id),
          m_time(time),
          m_level(level),
          m_logger(logger) {}
    const char *LogEvent::getFileName() const { return m_fileName; }
    int32_t LogEvent::getLine() const { return m_line; }
    uint32_t LogEvent::getElapse() const { return m_elapse; }
    uint32_t LogEvent::getThreadId() const { return m_threadId; }
    uint32_t LogEvent::getFiberId() const { return m_fiberId; }
    uint64_t LogEvent::getTime() const { return m_time; }
    std::string LogEvent::getMessage() const { return m_messageSS.str(); }
    std::stringstream &LogEvent::getSS() { return m_messageSS; }
    std::shared_ptr<Logger> LogEvent::getLogger() const { return m_logger; }
    LogLevel::Level LogEvent::getLevel() const { return m_level; }

    std::string LogEvent::getRelativeFileName() const
    {
        if (m_fileName == nullptr)
        {
            return "";
        }

        std::string file_name = m_fileName;
        // 查找文件路径中最后一个'/'的位置
        size_t pos = file_name.find_last_of('/');
        if (pos != std::string::npos)
        {
            // 查找 "sylar/" 子字符串
            size_t sylar_pos = file_name.find("sylar/");
            if (sylar_pos != std::string::npos)
            {
                return file_name.substr(sylar_pos+6);
            }
            // 如果没有找到 "sylar/"，则返回文件名部分
            return file_name.substr(pos + 1);
        }
        return file_name;
    }
    void LogEvent::format(const char *fmt, ...)
    {
        va_list al;
        va_start(al, fmt);
        format(fmt, al);
        va_end(al);
    }
    void LogEvent::format(const char *fmt, va_list al)
    {
        char *buf = nullptr;
        int len = vasprintf(&buf, fmt, al);
        if (len != -1)
        {
            m_messageSS << std::string(buf, len);
            free(buf);
        }
    }
    LogEventWrap::LogEventWrap(LogEvent::ptr event)
        : m_event(event) {}

    LogEventWrap::~LogEventWrap()
    {
        m_event->getLogger()->log(m_event->getLevel(), m_event);
    }
    LogEvent::ptr LogEventWrap::getEvent() const { return m_event; }
    std::stringstream &LogEventWrap::getSS() { return m_event->getSS(); }
}