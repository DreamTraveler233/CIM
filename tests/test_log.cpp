#include "logger.hpp"
#include "logger_manager.hpp"
#include "config.hpp"
#include <yaml-cpp/yaml.h>
#include <iostream>
#include <cassert>

// 测试日志系统
void test_log_system()
{
    std::cout << "=================== 测试日志系统 ===================" << std::endl;

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

int main(int argc, char **argv)
{
    std::cout << "开始执行日志模块全面测试" << std::endl;

    test_log_system();
    test_logger_creation();
    test_log_formatter();

    std::cout << "=================== 日志模块所有测试通过 ===================" << std::endl;
    return 0;
}