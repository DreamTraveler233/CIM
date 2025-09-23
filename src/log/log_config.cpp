#include "config.hpp"
#include "logger_manager.hpp"
#include "log.hpp"

namespace sylar
{
    // 用于存储所有的日志定义
    auto g_log_defines = sylar::Config::Lookup("logs", std::set<LogDefine>(), "logs config");

    // 静态变量初始化在main函数之前
    struct LogIniter
    {
        // 注册配置变更时的回调函数
        LogIniter()
        {
            g_log_defines->addListener([](const std::set<LogDefine> &old_val,
                                          const std::set<LogDefine> &new_val)
                                       {
                    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "on_logger_conf_changed";
                    for (auto &i : new_val)
                    {
                        // 新增以及修改日志器
                        // 查找日志器，如果找不到则创建并添加到日志器管理器中
                        auto logger = SYLAR_LOG_NAME(i.name);
                        logger->setLevel(i.level); // 设置该日志器的日志级别
                        if (!i.formatter.empty())
                        {
                            // 解析并设置格式化器，如果格式错误，则使用内置的默认格式
                            logger->setFormatter(i.formatter);
                        }
                        logger->clearAppender();
                        for (auto &j : i.appenders)
                        {
                            LogAppender::ptr ap;
                            if (j.type == 1)
                            {
                                ap = std::make_shared<FileLogAppender>(j.file);
                            }
                            else if (j.type == 2)
                            {
                                ap = std::make_shared<StdoutLogAppender>();
                            }
                            else
                            {
                                SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "type not FileLogAppender or StdoutLogAppender";
                            }

                            // 如果appender配置中有level，则使用它，否则使用logger的level
                            if (j.level != LogLevel::Level::UNKNOWN)
                            {
                                ap->setLevel(j.level);
                            }
                            else
                            {
                                ap->setLevel(i.level);
                            }

                            // 如果appender配置中有formatter，则使用它
                            if (!j.formatter.empty())
                            {
                                auto fmt = std::make_shared<LogFormatter>(j.formatter);
                                if (!fmt->isError())
                                {
                                    ap->setFormatter(fmt);
                                }
                            }

                            logger->addAppender(ap);
                        }
                    }
                    for (auto &i : old_val)
                    {
                        auto it = new_val.find(i);
                        if (it == new_val.end())
                        {
                            // 删除配置
                            // 如果在新配置中找不到当前旧配置项，说明这个配置项被删除了
                            auto logger = SYLAR_LOG_NAME(i.name);
                            logger->setLevel((LogLevel::Level)100); // 设置日志级别为100，表示关闭日志
                            logger->clearAppender();                // 清空所有appender
                        }
                    } });
        }
    };

    static LogIniter __log_init;
}