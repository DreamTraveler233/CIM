#include "thread.hpp"
#include "log.hpp"
#include "lock.hpp"
#include "config.hpp"
#include <yaml-cpp/yaml.h>
#include <vector>

auto g_logger = SYLAR_LOG_ROOT();

int count = 0;
// sylar::RWMutex rw_mutex;
sylar::Mutex mutex;
void func1()
{
    SYLAR_LOG_INFO(g_logger) << "name: " << sylar::Thread::GetName()
                             << "this.name: " << sylar::Thread::GetThis()->getName()
                             << " id: " << sylar::GetThreadId()
                             << " this.id: " << sylar::Thread::GetThis()->getId();
    // std::this_thread::sleep_for(std::chrono::seconds(360));
    for (int i = 0; i < 1000000; ++i)
    {
        // 获取写锁
        // sylar::RWMutex::WriteLock lock(rw_mutex);
        sylar::Mutex::Lock lock(mutex);
        count++;
    }
}

void func2()
{
    while (true)
    {
        SYLAR_LOG_INFO(g_logger) << "************************************";
    }
}

void func3()
{
    while (true)
    {
        SYLAR_LOG_INFO(g_logger) << "====================================";
    }
}

void func4()
{
    while (true)
    {
        std::cout << "***********************" << std::endl;
    }
}
void func5()
{
    while (true)
    {
        std::cout << "=======================" << std::endl;
    }
}

int main(int argc, char **argv)
{
    SYLAR_LOG_INFO(g_logger) << "thread test begin";
    YAML::Node root = YAML::LoadFile("/home/szy/code/sylar/bin/config/log.yaml");
    sylar::Config::LoadFromYaml(root);
    std::vector<sylar::Thread::ptr> thrs;
    for (int i = 0; i < 2; ++i)
    {
        sylar::Thread::ptr thr(new sylar::Thread(&func2, "name_" + std::to_string(i * 2)));
        sylar::Thread::ptr thr2(new sylar::Thread(&func3, "name_" + std::to_string(i * 2 + 1)));
        thrs.push_back(thr);
        thrs.push_back(thr2);
    }

    for (size_t i = 0; i < thrs.size(); ++i)
    {
        thrs[i]->join();
    }
    SYLAR_LOG_INFO(g_logger) << "thread test begin";
    SYLAR_LOG_INFO(g_logger) << "count = " << count;
    return 0;
}