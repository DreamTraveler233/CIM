#include "macro.hpp"
#include "iomanager.hpp"
#include "config.hpp"

void test()
{
    static auto g_logger = SYLAR_LOG_NAME("system");

    YAML::Node test = YAML::LoadFile("/home/szy/code/sylar/bin/config/log.yaml");
    sylar::Config::LoadFromYaml(test);

    // sylar::FileLogAppender::ptr file_ap(new sylar::FileLogAppender("/home/szy/code/sylar/bin/log/test.log"));
    // file_ap->getLogFile()->setRotateType(sylar::RotateType::Minute);
    // g_logger->addAppender(file_ap);

    for (int i = 0; i < 10000; ++i)
    {
        SYLAR_LOG_INFO(g_logger) << "第 " << i << " 条日志";
        sleep(1);
    }

    // // 加载日志配置文件
    // YAML::Node root = YAML::LoadFile("/home/szy/code/sylar/bin/log/log.yaml");
    // sylar::Config::LoadFromYaml(root);
}

int main(int argc, char **argv)
{
    sylar::IOManager iom(2);
    iom.schedule(test);
    return 0;
}