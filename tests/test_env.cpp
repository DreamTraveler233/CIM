#include "env.hpp"
#include <unistd.h>
#include <iostream>
#include <fstream>

struct A
{
    A()
    {
        std::ifstream ifs("/proc/" + std::to_string(getpid()) + "/cmdline", std::ios::binary);
        std::string content;
        content.resize(4096);

        ifs.read(&content[0], content.size());
        content.resize(ifs.gcount());

        for (size_t i = 0; i < content.size(); ++i)
        {
            std::cout << i << " - " << content[i] << " - " << (int)content[i] << std::endl;
        }
    }
};

//A a;

int main(int argc, char **argv)
{
    std::cout << "argc=" << argc << std::endl;
    CIM::EnvMgr::GetInstance()->addHelp("s", "start with the terminal");
    CIM::EnvMgr::GetInstance()->addHelp("d", "run as daemon");
    CIM::EnvMgr::GetInstance()->addHelp("p", "print help");
    if (!CIM::EnvMgr::GetInstance()->init(argc, argv))
    {
        CIM::EnvMgr::GetInstance()->printHelp();
        return 0;
    }

    std::cout << "exe=" << CIM::EnvMgr::GetInstance()->getExe() << std::endl;
    std::cout << "cwd=" << CIM::EnvMgr::GetInstance()->getCwd() << std::endl;

    std::cout << "path=" << CIM::EnvMgr::GetInstance()->getEnv("PATH", "xxx") << std::endl;
    std::cout << "test=" << CIM::EnvMgr::GetInstance()->getEnv("TEST", "") << std::endl;
    std::cout << "set env " << CIM::EnvMgr::GetInstance()->setEnv("TEST", "yy") << std::endl;
    std::cout << "test=" << CIM::EnvMgr::GetInstance()->getEnv("TEST", "") << std::endl;
    if (CIM::EnvMgr::GetInstance()->has("p"))
    {
        CIM::EnvMgr::GetInstance()->printHelp();
    }
    return 0;
}