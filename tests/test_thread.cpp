#include "thread.hpp"
#include "log.hpp"
#include "lock.hpp"
#include <vector>
#include <iostream>
#include <atomic>

// 创建多个logger用于测试
auto g_logger1 = SYLAR_LOG_NAME("test_logger1");
auto g_logger2 = SYLAR_LOG_NAME("test_logger2");
auto g_logger3 = SYLAR_LOG_NAME("test_logger3");

// 用于检测竞态条件的原子计数器
std::atomic<int> g_log_count{0};
std::atomic<int> g_appender_count{0};
std::atomic<int> g_level_change_count{0};

// 测试logger的基本日志功能
void testBasicLogging() {
    for (int i = 0; i < 1000; ++i) {
        SYLAR_LOG_DEBUG(g_logger1) << "Debug message " << i << " from thread " << sylar::Thread::GetThis()->getName();
        SYLAR_LOG_INFO(g_logger2) << "Info message " << i << " from thread " << sylar::Thread::GetThis()->getName();
        SYLAR_LOG_WARN(g_logger3) << "Warning message " << i << " from thread " << sylar::Thread::GetThis()->getName();
        g_log_count++;
    }
}

// 测试logger的appender操作
void testAppenderOperations() {
    auto logger = SYLAR_LOG_NAME("appender_test");
    
    for (int i = 0; i < 100; ++i) {
        // 添加appender
        auto appender = std::make_shared<sylar::StdoutLogAppender>();
        logger->addAppender(appender);
        g_appender_count++;
        
        // 删除appender
        logger->delAppender(appender);
        g_appender_count++;
        
        // 再添加一个appender
        auto file_appender = std::make_shared<sylar::FileLogAppender>("thread_test.log");
        logger->addAppender(file_appender);
        g_appender_count++;
        
        // 清空appender
        logger->clearAppender();
        g_appender_count++;
    }
}

// 测试logger级别变更
void testLevelChanges() {
    auto logger = SYLAR_LOG_NAME("level_test");
    
    for (int i = 0; i < 100; ++i) {
        // 更改日志级别
        logger->setLevel(sylar::LogLevel::Level::DEBUG);
        g_level_change_count++;
        
        logger->setLevel(sylar::LogLevel::Level::INFO);
        g_level_change_count++;
        
        logger->setLevel(sylar::LogLevel::Level::WARN);
        g_level_change_count++;
        
        logger->setLevel(sylar::LogLevel::Level::ERROR);
        g_level_change_count++;
        
        // 检查当前级别
        auto level = logger->getLevel();
        (void)level; // 避免未使用变量警告
        g_level_change_count++;
    }
}

// 测试logger格式化器变更
void testFormatterChanges() {
    auto logger = SYLAR_LOG_NAME("formatter_test");
    
    for (int i = 0; i < 100; ++i) {
        // 更改格式化器
        logger->setFormatter("%d [%p] <%c> %m%n");
        SYLAR_LOG_INFO(logger) << "Message with custom formatter " << i;
        
        logger->setFormatter("%d %t %m%n");
        SYLAR_LOG_INFO(logger) << "Message with another formatter " << i;
        
        // 使用默认格式化器
        //logger->setFormatter(nullptr);
        //SYLAR_LOG_INFO(logger) << "Message with default formatter " << i;
    }
}

// 大量并发日志输出测试
void testConcurrentLogging() {
    auto logger = SYLAR_LOG_NAME("concurrent_test");
    
    for (int i = 0; i < 1000; ++i) {
        SYLAR_LOG_FMT_INFO(logger, "Formatted log message %d from thread %s", 
                          i, sylar::Thread::GetThis()->getName().c_str());
        g_log_count++;
    }
}

int main(int argc, char** argv) {
    std::cout << "Starting thread safety test for log system..." << std::endl;
    
    // 创建多个线程执行不同的测试任务
    std::vector<sylar::Thread::ptr> threads;
    
    // 创建测试基本日志功能的线程
    for (int i = 0; i < 5; ++i) {
        sylar::Thread::ptr thr(new sylar::Thread(testBasicLogging, "basic_log_" + std::to_string(i)));
        threads.push_back(thr);
    }
    
    // 创建测试appender操作的线程
    for (int i = 0; i < 3; ++i) {
        sylar::Thread::ptr thr(new sylar::Thread(testAppenderOperations, "appender_test_" + std::to_string(i)));
        threads.push_back(thr);
    }
    
    // 创建测试级别变更的线程
    for (int i = 0; i < 3; ++i) {
        sylar::Thread::ptr thr(new sylar::Thread(testLevelChanges, "level_test_" + std::to_string(i)));
        threads.push_back(thr);
    }
    
    // 创建测试格式化器变更的线程
    for (int i = 0; i < 2; ++i) {
        sylar::Thread::ptr thr(new sylar::Thread(testFormatterChanges, "formatter_test_" + std::to_string(i)));
        threads.push_back(thr);
    }
    
    // 创建大量并发日志输出测试的线程
    for (int i = 0; i < 5; ++i) {
        sylar::Thread::ptr thr(new sylar::Thread(testConcurrentLogging, "concurrent_test_" + std::to_string(i)));
        threads.push_back(thr);
    }
    
    // 等待所有线程完成
    for (auto& thr : threads) {
        thr->join();
    }
    
    std::cout << "Thread safety test completed!" << std::endl;
    std::cout << "Total log operations: " << g_log_count.load() << std::endl;
    std::cout << "Total appender operations: " << g_appender_count.load() << std::endl;
    std::cout << "Total level change operations: " << g_level_change_count.load() << std::endl;
    
    // 如果程序能正常结束而没有崩溃或死锁，说明基本线程安全
    std::cout << "If the program reached here without crash or deadlock, basic thread safety is achieved." << std::endl;
    std::cout << "Note: This test does not guarantee complete thread safety, especially with container operations." << std::endl;
    
    return 0;
}