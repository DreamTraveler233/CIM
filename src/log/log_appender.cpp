#include "log_appender.hpp"
#include "yaml-cpp/yaml.h"
#include "macro.hpp"

namespace sylar
{
    LogAppender::~LogAppender() {}
    void LogAppender::setFormatter(LogFormatter::ptr formatter)
    {
        SYLAR_ASSERT(formatter);
        MutexType::Lock lock(m_mutex);
        m_formatter = formatter;
    }
    LogFormatter::ptr LogAppender::getFormatter() const
    {
        MutexType::Lock lock(m_mutex);
        return m_formatter;
    }
    void LogAppender::setLevel(Level level)
    {
        SYLAR_ASSERT(level != Level::UNKNOWN);
        MutexType::Lock lock(m_mutex);
        m_level = level;
    }
    Level LogAppender::getLevel() const
    {
        MutexType::Lock lock(m_mutex);
        return m_level;
    }

    void StdoutLogAppender::log(LogEvent::ptr event)
    {
        SYLAR_ASSERT(event);
        if (event->getLevel() >= m_level)
        {
            MutexType::Lock lock(m_mutex);
            // 将日志事件（event）格式化后输出到标准输出（cout）
            std::cout << m_formatter->format(event);
        }
    }

    std::string StdoutLogAppender::toYamlString()
    {
        MutexType::Lock lock(m_mutex);
        YAML::Node node;
        node["type"] = "StdoutLogAppender";
        node["level"] = LogLevel::ToString(m_level);
        if (m_formatter)
        {
            node["formatter"] = m_formatter->getPattern();
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }

    FileLogAppender::FileLogAppender(const std::string &fileName)
    {
        SYLAR_ASSERT(!fileName.empty())
        m_logFile = LogFileManager::GetInstance()->getLogFile(fileName);
        m_logFile->openFile();
    }

    void FileLogAppender::log(LogEvent::ptr event)
    {
        SYLAR_ASSERT(event);
        if (event->getLevel() >= m_level)
        {
            MutexType::Lock lock(m_mutex);
            if (m_logFile)
            {
                std::string formatted_msg = m_formatter->format(event);
                m_logFile->writeLog(formatted_msg);
            }
        }
    }

    std::string FileLogAppender::toYamlString()
    {
        MutexType::Lock lock(m_mutex);
        YAML::Node node;
        node["type"] = "FileLogAppender";
        node["file"] = m_logFile->getFilePath();
        node["level"] = LogLevel::ToString(m_level);
        if (m_formatter)
        {
            node["formatter"] = m_formatter->getPattern();
        }
        node["rotate_type"] = LogFile::rotateTypeToString(m_logFile->getRotateType());
        std::stringstream ss;
        ss << node;
        return ss.str();
    }

    LogFile::ptr FileLogAppender::getLogFile() const
    {
        return m_logFile;
    }
}