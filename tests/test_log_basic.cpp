#include "logger.hpp"
#include "logger_manager.hpp"
#include "config.hpp"
#include "log_appender.hpp"
#include <yaml-cpp/yaml.h>
#include <iostream>
#include <cassert>
#include <fstream>

// 测试日志系统
void test_log_system()
{
    std::cout << "=================== 日志系统基本 ===================" << std::endl;

    auto system_log = SYLAR_LOG_NAME("system");
    auto root_log = SYLAR_LOG_NAME("root");

    SYLAR_LOG_DEBUG(system_log) << "debug message for system";
    SYLAR_LOG_INFO(system_log) << "info message for system";
    SYLAR_LOG_ERROR(system_log) << "error message for system";
    SYLAR_LOG_INFO(root_log) << "info message for root";

    // 测试日志管理器
    auto logger_manager = sylar::loggerMgr::getInstance();
    assert(logger_manager != nullptr);

    auto system_logger = logger_manager->getLogger("system");
    auto root_logger = logger_manager->getLogger("root");
    auto default_logger = logger_manager->getLogger("nonexistent");

    assert(system_logger != nullptr);
    assert(root_logger != nullptr);
    // 修改测试逻辑：验证新创建的logger有正确的根logger
    assert(default_logger != nullptr);
    assert(default_logger->getRoot() == root_logger);

    std::cout << "日志系统基本功能测试通过" << std::endl;

    // 测试YAML配置加载
    std::string before_config = logger_manager->toYamlString();
    YAML::Node root = YAML::LoadFile("/home/szy/code/sylar/bin/config/log.yaml");
    sylar::Config::LoadFromYaml(root);
    std::string after_config = logger_manager->toYamlString();

    // 配置应该发生变化
    assert(before_config != after_config);

    std::cout << "日志系统YAML配置加载测试通过" << std::endl;

    // 测试配置后的日志输出
    SYLAR_LOG_DEBUG(system_log) << "debug message after config";
    SYLAR_LOG_INFO(system_log) << "info message after config";

    std::cout << "日志系统配置后输出测试通过" << std::endl;
}

void test_logger_creation()
{
    std::cout << "=================== 测试日志器创建 ===================" << std::endl;

    // 测试获取已存在的logger
    auto logger1 = SYLAR_LOG_NAME("test_logger");
    auto logger2 = SYLAR_LOG_NAME("test_logger");

    // 应该是同一个实例
    assert(logger1 == logger2);

    // 测试日志级别设置
    logger1->setLevel(sylar::LogLevel::Level::ERROR);
    assert(logger1->getLevel() == sylar::LogLevel::Level::ERROR);

    std::cout << "日志器创建和级别设置测试通过" << std::endl;
}

void test_log_formatter()
{
    std::cout << "=================== 测试日志格式化器 ===================" << std::endl;

    auto test_logger = SYLAR_LOG_NAME("formatter_test");

    // 设置自定义格式
    std::string custom_format = "%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%l%T%m%n";
    test_logger->setFormatter(custom_format);

    // 测试日志输出
    SYLAR_LOG_INFO(test_logger) << "测试自定义格式";

    std::cout << "日志格式化器测试通过" << std::endl;
}

void test_log_appender()
{
    std::cout << "=================== 测试日志附加器 ===================" << std::endl;

    auto test_logger = SYLAR_LOG_NAME("appender_test");

    // 创建并添加文件附加器
    auto file_appender = std::make_shared<sylar::FileLogAppender>("test_log.txt");
    test_logger->addAppender(file_appender);

    // 测试日志输出到文件
    SYLAR_LOG_INFO(test_logger) << "测试文件附加器";

    // 创建并添加控制台附加器
    auto stdout_appender = std::make_shared<sylar::StdoutLogAppender>();
    test_logger->addAppender(stdout_appender);

    SYLAR_LOG_DEBUG(test_logger) << "测试多个附加器";

    // 测试删除附加器
    test_logger->delAppender(file_appender);
    SYLAR_LOG_WARN(test_logger) << "测试删除附加器后";

    // 清空附加器
    test_logger->clearAppender();

    std::cout << "日志附加器测试通过" << std::endl;
}

void test_log_level()
{
    std::cout << "=================== 测试日志级别控制 ===================" << std::endl;

    auto test_logger = SYLAR_LOG_NAME("level_test");

    // 设置日志级别为ERROR
    test_logger->setLevel(sylar::LogLevel::Level::ERROR);

    // DEBUG和INFO级别应该不输出（因为我们无法直接验证输出，但可以通过其他方式验证）
    assert(test_logger->getLevel() == sylar::LogLevel::Level::ERROR);

    // 测试不同级别的日志事件创建
    auto event_debug = std::make_shared<sylar::LogEvent>(test_logger, sylar::LogLevel::Level::DEBUG,
                                                         __FILE__, __LINE__, 0, 0, 0, time(0));
    auto event_error = std::make_shared<sylar::LogEvent>(test_logger, sylar::LogLevel::Level::ERROR,
                                                         __FILE__, __LINE__, 0, 0, 0, time(0));

    event_debug->getSS() << "这是一条DEBUG消息";
    event_error->getSS() << "这是一条ERROR消息";

    // 调用日志方法
    test_logger->debug(event_debug);
    test_logger->error(event_error);

    std::cout << "日志级别控制测试通过" << std::endl;
}

void test_log_event()
{
    std::cout << "=================== 测试日志事件 ===================" << std::endl;

    auto test_logger = SYLAR_LOG_NAME("event_test");

    // 创建日志事件
    auto event = std::make_shared<sylar::LogEvent>(test_logger, sylar::LogLevel::Level::INFO,
                                                   "test_file.cpp", 123, 1000, 12345, 1, time(0));

    // 测试格式化功能
    event->format("这是一个格式化消息，参数1: %d, 参数2: %s", 42, "test");

    // 测试获取事件属性
    assert(std::string(event->getFileName()) == "test_file.cpp");
    assert(event->getLine() == 123);
    assert(event->getThreadId() == 12345);
    assert(event->getFiberId() == 1);

    // 输出日志
    test_logger->info(event);

    std::cout << "日志事件测试通过" << std::endl;
}

int main(int argc, char **argv)
{
    std::cout << "开始执行日志模块全面测试" << std::endl;

    test_log_system();
    test_logger_creation();
    test_log_formatter();
    test_log_appender();
    test_log_level();
    test_log_event();

    std::cout << "=================== 日志模块所有测试通过 ===================" << std::endl;
    return 0;
}