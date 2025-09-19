#include "logger.hpp"
#include "yaml-cpp/yaml.h"

namespace sylar
{
    Logger::Logger(const std::string &name)
        : m_name(name),
          m_level(LogLevel::Level::DEBUG)
    {
        // 时间 线程号 协程号 [日志级别] 文件名 行号 日志信息 回车
        m_formatter = std::make_shared<LogFormatter>("%d%T[%t]%T[%F]%T[%p]%T[%c]%T<%f:%l>%T%m%n");
    }

    void Logger::log(LogLevel::Level level, LogEvent::ptr event)
    {
        if (level >= m_level)
        {
            if (!m_appenders.empty())
            {
                for (auto &i : m_appenders)
                {
                    i->log(event);
                }
            }
            else if (m_root)
            {
                m_root->log(level, event);
            }
        }
    }

    void Logger::debug(LogEvent::ptr event)
    {
        log(LogLevel::Level::DEBUG, event);
    }
    void Logger::info(LogEvent::ptr event)
    {
        log(LogLevel::Level::INFO, event);
    }
    void Logger::warn(LogEvent::ptr event)
    {
        log(LogLevel::Level::WARN, event);
    }
    void Logger::error(LogEvent::ptr event)
    {
        log(LogLevel::Level::ERROR, event);
    }
    void Logger::fatal(LogEvent::ptr event)
    {
        log(LogLevel::Level::FATAL, event);
    }

    void Logger::addAppender(LogAppender::ptr appender)
    {
        // 当appender没有自己的formatter时，则使用logger的formatter
        if (!appender->getFormatter())
        {
            appender->setFormatter(m_formatter);
        }
        m_appenders.push_back(appender);
    }

    void Logger::delAppender(LogAppender::ptr appender)
    {
        for (auto it = m_appenders.begin(); it != m_appenders.end(); ++it)
        {
            if (*it == appender)
            {
                m_appenders.erase(it);
                break;
            }
        }
    }

    void Logger::clearAppender()
    {
        m_appenders.clear();
    }
    LogLevel::Level Logger::getLevel() const { return m_level; }
    void Logger::setLevel(LogLevel::Level level) { m_level = level; }
    const std::string &Logger::getName() const { return m_name; }
    void Logger::setFormatter(LogFormatter::ptr val) { m_formatter = val; }
    LogFormatter::ptr Logger::getFormatter() const { return m_formatter; }
    Logger::ptr Logger::getRoot() const { return m_root; }
    std::string Logger::toYamlString()
    {
        YAML::Node node;
        node["name"] = m_name;
        node["level"] = LogLevel::ToString(m_level);
        if (m_formatter)
        {
            node["formatter"] = m_formatter->getPattern();
        }
        for (auto &i : m_appenders)
        {
            node["appenders"].push_back(YAML::Load(i->toYamlString()));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }

    /**
     * @brief 设置日志格式化器
     * @param val 格式化字符串，用于定义日志输出格式
     *
     * 该函数根据传入的格式化字符串创建一个新的LogFormatter对象，
     * 并检查该格式化器是否有效。如果有效，则将其设置为当前Logger的格式化器；
     * 如果无效，则输出错误信息并返回。
     */
    void Logger::setFormatter(const std::string &val)
    {
        auto new_val = std::make_shared<LogFormatter>(val);
        if (new_val->isError())
        {
            std::cout << "Logger setFormatter name=" << m_name
                      << " value=" << val << " invalid formatter"
                      << std::endl;
            return;
        }
        m_formatter = new_val;
    }
}