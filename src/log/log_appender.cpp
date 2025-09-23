#include "log_appender.hpp"
#include "yaml-cpp/yaml.h"

namespace sylar
{
    LogAppender::~LogAppender() {}
    void LogAppender::setFormatter(LogFormatter::ptr formatter)
    {
        MutexType::Lock lock(m_mutex);
        m_formatter = formatter;
    }
    LogFormatter::ptr LogAppender::getFormatter() const
    {
        MutexType::Lock lock(m_mutex);
        return m_formatter;
    }
    void LogAppender::setLevel(LogLevel::Level level)
    {
        MutexType::Lock lock(m_mutex);
        m_level = level;
    }
    LogLevel::Level LogAppender::getLevel() const
    {
        MutexType::Lock lock(m_mutex);
        return m_level;
    }

    void StdoutLogAppender::log(LogEvent::ptr event)
    {
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
        : m_fileName(fileName)
    {
        reopen();
    }

    FileLogAppender::~FileLogAppender()
    {
        if (m_fileStream.is_open())
        {
            m_fileStream.close();
        }
    }

    void FileLogAppender::log(LogEvent::ptr event)
    {
        if (event->getLevel() >= m_level)
        {
            MutexType::Lock lock(m_mutex);
            // 将日志事件（event）格式化后输出到文件流
            m_fileStream << m_formatter->format(event);
        }
    }

    std::string FileLogAppender::toYamlString()
    {
        MutexType::Lock lock(m_mutex);
        YAML::Node node;
        node["type"] = "FileLogAppender";
        node["file"] = m_fileName;
        node["level"] = LogLevel::ToString(m_level);
        if (m_formatter)
        {
            node["formatter"] = m_formatter->getPattern();
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }

    bool FileLogAppender::reopen()
    {
        MutexType::Lock lock(m_mutex);
        if (m_fileStream.is_open())
        {
            m_fileStream.close();
        }
        // m_fileStream.open(m_fileName, std::ios::app);
        m_fileStream.open(m_fileName);

        return m_fileStream.is_open();
    }
}