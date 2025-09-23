#pragma once

#include "log_event.hpp"
#include "log_formatter.hpp"
#include "lock.hpp"
#include <fstream>
#include <string>

namespace sylar
{
    class LogAppender
    {
    public:
        using ptr = std::shared_ptr<LogAppender>;

        virtual ~LogAppender();

        virtual void log(LogEvent::ptr event) = 0;
        virtual std::string toYamlString() = 0;

        void setFormatter(LogFormatter::ptr formatter);
        LogFormatter::ptr getFormatter() const;
        void setLevel(LogLevel::Level level);
        LogLevel::Level getLevel() const;

    protected:
        LogLevel::Level m_level = LogLevel::Level::DEBUG; // 日志级别
        LogFormatter::ptr m_formatter;                    // 日志格式器
        Mutex m_mutex;                                    // 互斥锁
    };

    class StdoutLogAppender : public LogAppender
    {
    public:
        using ptr = std::shared_ptr<StdoutLogAppender>;

        virtual void log(LogEvent::ptr event) override;
        virtual std::string toYamlString() override;
    };

    class FileLogAppender : public LogAppender
    {
    public:
        using ptr = std::shared_ptr<FileLogAppender>;

        FileLogAppender(const std::string &fileName);
        ~FileLogAppender();

        virtual void log(LogEvent::ptr event) override;
        virtual std::string toYamlString() override;

        bool reopen();

    private:
        std::string m_fileName;     // 文件名
        std::ofstream m_fileStream; // 文件流
    };
}