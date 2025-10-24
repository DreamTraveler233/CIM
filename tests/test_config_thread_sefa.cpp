#include "config.hpp"
#include <yaml-cpp/yaml.h>
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <atomic>
#include <cassert>

// 定义测试配置项
auto g_config_int = CIM::Config::Lookup<int>("test.int", 0, "test int config");
auto g_config_string = CIM::Config::Lookup<std::string>("test.string", "default", "test string config");
auto g_config_vector = CIM::Config::Lookup<std::vector<int>>("test.vector", std::vector<int>{1, 2, 3}, "test vector config");

// 原子计数器，用于统计操作次数
std::atomic<int> g_value_changes(0);    // 统计配置值改变的次数
std::atomic<int> g_callback_calls(0);   // 统计回调函数调用次数
std::atomic<bool> g_test_running(true); // 测试是否继续运行

// 回调函数，用于测试回调是否正常工作
void onConfigChange(const int &old_val, const int &new_val)
{
    g_callback_calls++;
    // 注意：在多线程环境中，当我们执行到这里时，配置值可能已经被其他线程修改了
    // 所以我们不能断言 new_val == g_config_int->getValue()
    // 这不是错误，而是多线程环境下的正常现象
}

/**
 * 测试线程函数 - 执行写操作
 * 写线程负责修改配置项的值，通过频繁调用setValue和fromString方法来测试配置项在写操作时的线程安全性。
 */
void writer_thread(int thread_id, int operations)
{
    for (int i = 0; i < operations && g_test_running; ++i)
    {
        int new_value = thread_id * 1000 + i;
        g_config_int->setValue(new_value);
        g_value_changes++;

        // 同时测试fromString方法
        std::string str_val = std::to_string(thread_id * 2000 + i);
        g_config_string->fromString(str_val);

        // 短暂休眠以增加并发冲突的可能性
        std::this_thread::sleep_for(std::chrono::microseconds(10));
    }
}

/**
 * 测试线程函数 - 执行读操作
 * 读线程负责读取配置项的值，通过频繁调用getValue方法来测试配置项在读操作时的线程安全性。
 */
void reader_thread(int thread_id, int operations)
{
    for (int i = 0; i < operations && g_test_running; ++i)
    {
        // 读取各种配置项的值
        int int_val = g_config_int->getValue();
        std::string str_val = g_config_string->getValue();
        std::vector<int> vec_val = g_config_vector->getValue();

        // 简单验证值的存在性
        // 注意：这里不做严格的值验证，因为其他线程可能随时修改值
        (void)int_val; // 避免未使用变量警告

        // 短暂休眠
        std::this_thread::sleep_for(std::chrono::microseconds(5));
    }
}

/**
 * 测试线程函数 - 执行toString操作
 * toString线程负责测试配置项的序列化功能，通过频繁调用toString方法来测试配置项在线程环境下的序列化操作。
 */
void tostring_thread(int thread_id, int operations)
{
    for (int i = 0; i < operations && g_test_running; ++i)
    {
        // 测试toString方法
        std::string int_str = g_config_int->toString();
        std::string str_str = g_config_string->toString();
        std::string vec_str = g_config_vector->toString();

        // 简单验证结果不为空
        assert(!int_str.empty());
        assert(!str_str.empty());
        assert(!vec_str.empty());

        // 短暂休眠
        std::this_thread::sleep_for(std::chrono::microseconds(7));
    }
}

void test_thread_safety()
{
    const int num_writer_threads = 4;       // 写线程数
    const int num_reader_threads = 6;       // 读线程数
    const int num_tostring_threads = 4;     // toString线程数
    const int operations_per_thread = 1000; // 每个线程执行操作的次数

    std::cout << "开始线程安全测试..." << std::endl;
    std::cout << "启动 " << num_writer_threads << " 个写线程，"
              << num_reader_threads << " 个读线程，"
              << num_tostring_threads << " 个toString线程" << std::endl;
    std::cout << "每个线程执行 " << operations_per_thread << " 次操作" << std::endl;

    // 注册回调函数
    uint64_t callback_id = g_config_int->addListener(onConfigChange);

    std::vector<std::thread> threads;

    // 创建写线程
    for (int i = 0; i < num_writer_threads; ++i)
    {
        threads.emplace_back(writer_thread, i, operations_per_thread);
    }

    // 创建读线程
    for (int i = 0; i < num_reader_threads; ++i)
    {
        threads.emplace_back(reader_thread, i, operations_per_thread);
    }

    // 创建toString线程
    for (int i = 0; i < num_tostring_threads; ++i)
    {
        threads.emplace_back(tostring_thread, i, operations_per_thread);
    }

    // 等待所有线程完成
    for (auto &t : threads)
    {
        t.join();
    }

    // 停止测试
    g_test_running = false;

    // 删除回调函数
    g_config_int->delListener(callback_id);

    // 输出测试结果
    std::cout << "测试完成!" << std::endl;
    std::cout << "总共执行写操作: " << g_value_changes.load() << " 次" << std::endl;
    std::cout << "回调函数被调用: " << g_callback_calls.load() << " 次" << std::endl;

    // 验证最终状态
    std::string final_string = g_config_string->getValue();
    std::string final_string_from_tostring = g_config_string->toString();

    std::cout << "最终配置值:" << std::endl;
    std::cout << "  int: " << g_config_int->getValue() << std::endl;
    std::cout << "  string: " << final_string << std::endl;
    std::cout << "  string (from toString): " << final_string_from_tostring << std::endl;

    assert(!final_string.empty());
    assert(final_string == final_string_from_tostring);

    std::cout << "线程安全测试通过!" << std::endl;
}

int main(int argc, char **argv)
{
    std::cout << "开始执行配置模块线程安全测试" << std::endl;

    try
    {
        test_thread_safety();
        std::cout << "=================== 配置模块线程安全测试通过 ===================" << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "测试过程中发生异常: " << e.what() << std::endl;
        return 1;
    }
    catch (...)
    {
        std::cerr << "测试过程中发生未知异常" << std::endl;
        return 1;
    }

    return 0;
}