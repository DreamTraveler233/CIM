#include "application.hpp"
#include "tcp_server.hpp"
#include "daemon.hpp"
#include "config.hpp"
#include "env.hpp"
#include "macro.hpp"
#include "module.hpp"
#include "rock_stream.hpp"
#include "worker.hpp"
#include "ws_server.hpp"
#include "rock_server.hpp"
#include "name_server_module.hpp"
#include "fox_thread.hpp"
#include "redis.hpp"

#include <unistd.h>
#include <signal.h>

namespace CIM
{
    static Logger::ptr g_logger = CIM_LOG_NAME("system");

    static ConfigVar<std::string>::ptr g_server_work_path =
        Config::Lookup("server.work_path", std::string("/apps/work/sylar"), "server work path");

    static ConfigVar<std::string>::ptr g_server_pid_file =
        Config::Lookup("server.pid_file", std::string("sylar.pid"), "server pid file");

    static ConfigVar<std::string>::ptr g_service_discovery_zk =
        Config::Lookup("service_discovery.zk", std::string(""), "service discovery zookeeper");

    static ConfigVar<std::vector<TcpServerConf>>::ptr g_servers_conf = Config::Lookup("servers", std::vector<TcpServerConf>(), "http server config");

    Application *Application::s_instance = nullptr;

    Application::Application()
    {
        s_instance = this;
    }

    bool Application::init(int argc, char **argv)
    {
        m_argc = argc;
        m_argv = argv;

        EnvMgr::GetInstance()->addHelp("s", "start with the terminal");
        EnvMgr::GetInstance()->addHelp("d", "run as daemon");
        EnvMgr::GetInstance()->addHelp("c", "conf path default: ./conf");
        EnvMgr::GetInstance()->addHelp("p", "print help");

        bool is_print_help = false;
        if (!EnvMgr::GetInstance()->init(argc, argv))
        {
            is_print_help = true;
        }

        if (EnvMgr::GetInstance()->has("p"))
        {
            is_print_help = true;
        }

        std::string conf_path = EnvMgr::GetInstance()->getConfigPath();
        CIM_LOG_INFO(g_logger) << "load conf path:" << conf_path;
        Config::LoadFromConfDir(conf_path);

        ModuleMgr::GetInstance()->init();
        std::vector<Module::ptr> modules;
        ModuleMgr::GetInstance()->listAll(modules);

        for (auto i : modules)
        {
            i->onBeforeArgsParse(argc, argv);
        }

        if (is_print_help)
        {
            EnvMgr::GetInstance()->printHelp();
            return false;
        }

        for (auto i : modules)
        {
            i->onAfterArgsParse(argc, argv);
        }
        modules.clear();

        int run_type = 0;
        if (EnvMgr::GetInstance()->has("s"))
        {
            run_type = 1;
        }
        if (EnvMgr::GetInstance()->has("d"))
        {
            run_type = 2;
        }

        if (run_type == 0)
        {
            EnvMgr::GetInstance()->printHelp();
            return false;
        }

        std::string pidfile = g_server_work_path->getValue() + "/" + g_server_pid_file->getValue();
        if (FSUtil::IsRunningPidfile(pidfile))
        {
            CIM_LOG_ERROR(g_logger) << "server is running:" << pidfile;
            return false;
        }

        if (!FSUtil::Mkdir(g_server_work_path->getValue()))
        {
            CIM_LOG_FATAL(g_logger) << "create work path [" << g_server_work_path->getValue()
                                    << " errno=" << errno << " errstr=" << strerror(errno);
            return false;
        }
        return true;
    }

    bool Application::run()
    {
        bool is_daemon = EnvMgr::GetInstance()->has("d");
        return start_daemon(m_argc, m_argv,
                            std::bind(&Application::main, this, std::placeholders::_1,
                                      std::placeholders::_2),
                            is_daemon);
    }

    int Application::main(int argc, char **argv)
    {
        signal(SIGPIPE, SIG_IGN);
        CIM_LOG_INFO(g_logger) << "main";
        std::string conf_path = EnvMgr::GetInstance()->getConfigPath();
        Config::LoadFromConfDir(conf_path, true);
        {
            std::string pidfile = g_server_work_path->getValue() + "/" + g_server_pid_file->getValue();
            std::ofstream ofs(pidfile);
            if (!ofs)
            {
                CIM_LOG_ERROR(g_logger) << "open pidfile " << pidfile << " failed";
                return false;
            }
            ofs << getpid();
        }

        m_mainIOManager.reset(new IOManager(1, true, "main"));
        m_mainIOManager->schedule(std::bind(&Application::run_coroutine, this));
        m_mainIOManager->addTimer(2000, []()
                                  {
                                      // CIM_LOG_INFO(g_logger) << "hello";
                                  },
                                  true);
        m_mainIOManager->stop();
        return 0;
    }

    int Application::run_coroutine()
    {
        std::vector<Module::ptr> modules;
        ModuleMgr::GetInstance()->listAll(modules);
        bool has_error = false;
        for (auto &i : modules)
        {
            if (!i->onLoad())
            {
                CIM_LOG_ERROR(g_logger) << "module name="
                                        << i->getName() << " version=" << i->getVersion()
                                        << " filename=" << i->getFilename();
                has_error = true;
            }
        }
        if (has_error)
        {
            _exit(0);
        }

        WorkerMgr::GetInstance()->init();
        FoxThreadMgr::GetInstance()->init();
        FoxThreadMgr::GetInstance()->start();
        RedisMgr::GetInstance();

        auto http_confs = g_servers_conf->getValue();
        std::vector<TcpServer::ptr> svrs;
        for (auto &i : http_confs)
        {
            CIM_LOG_DEBUG(g_logger) << std::endl
                                    << LexicalCast<TcpServerConf, std::string>()(i);

            std::vector<Address::ptr> address;
            for (auto &a : i.address)
            {
                size_t pos = a.find(":");
                if (pos == std::string::npos)
                {
                    // CIM_LOG_ERROR(g_logger) << "invalid address: " << a;
                    address.push_back(UnixAddress::ptr(new UnixAddress(a)));
                    continue;
                }
                int32_t port = atoi(a.substr(pos + 1).c_str());
                // 127.0.0.1
                auto addr = IPAddress::Create(a.substr(0, pos).c_str(), port);
                if (addr)
                {
                    address.push_back(addr);
                    continue;
                }
                std::vector<std::pair<Address::ptr, uint32_t>> result;
                if (Address::GetInterfaceAddresses(result,
                                                   a.substr(0, pos)))
                {
                    for (auto &x : result)
                    {
                        auto ipaddr = std::dynamic_pointer_cast<IPAddress>(x.first);
                        if (ipaddr)
                        {
                            ipaddr->setPort(atoi(a.substr(pos + 1).c_str()));
                        }
                        address.push_back(ipaddr);
                    }
                    continue;
                }

                auto aaddr = Address::LookupAny(a);
                if (aaddr)
                {
                    address.push_back(aaddr);
                    continue;
                }
                CIM_LOG_ERROR(g_logger) << "invalid address: " << a;
                _exit(0);
            }
            IOManager *accept_worker = IOManager::GetThis();
            IOManager *io_worker = IOManager::GetThis();
            IOManager *process_worker = IOManager::GetThis();
            if (!i.accept_worker.empty())
            {
                accept_worker = WorkerMgr::GetInstance()->getAsIOManager(i.accept_worker).get();
                if (!accept_worker)
                {
                    CIM_LOG_ERROR(g_logger) << "accept_worker: " << i.accept_worker
                                            << " not exists";
                    _exit(0);
                }
            }
            if (!i.io_worker.empty())
            {
                io_worker = WorkerMgr::GetInstance()->getAsIOManager(i.io_worker).get();
                if (!io_worker)
                {
                    CIM_LOG_ERROR(g_logger) << "io_worker: " << i.io_worker
                                            << " not exists";
                    _exit(0);
                }
            }
            if (!i.process_worker.empty())
            {
                process_worker = WorkerMgr::GetInstance()->getAsIOManager(i.process_worker).get();
                if (!process_worker)
                {
                    CIM_LOG_ERROR(g_logger) << "process_worker: " << i.process_worker
                                            << " not exists";
                    _exit(0);
                }
            }

            TcpServer::ptr server;
            if (i.type == "http")
            {
                server.reset(new CIM::http::HttpServer(i.keepalive, process_worker, io_worker, accept_worker));
            }
            else if (i.type == "ws")
            {
                server.reset(new CIM::http::WSServer(process_worker, io_worker, accept_worker));
            }
            else if (i.type == "rock")
            {
                server.reset(new RockServer("rock",
                                            process_worker, io_worker, accept_worker));
            }
            else if (i.type == "nameserver")
            {
                server.reset(new RockServer("nameserver",
                                            process_worker, io_worker, accept_worker));
                ModuleMgr::GetInstance()->add(std::make_shared<ns::NameServerModule>());
            }
            else
            {
                CIM_LOG_ERROR(g_logger) << "invalid server type=" << i.type
                                        << LexicalCast<TcpServerConf, std::string>()(i);
                _exit(0);
            }
            if (!i.name.empty())
            {
                server->setName(i.name);
            }
            std::vector<Address::ptr> fails;
            if (!server->bind(address, fails, i.ssl))
            {
                for (auto &x : fails)
                {
                    CIM_LOG_ERROR(g_logger) << "bind address fail:"
                                            << *x;
                }
                _exit(0);
            }
            if (i.ssl)
            {
                if (!server->loadCertificates(i.cert_file, i.key_file))
                {
                    CIM_LOG_ERROR(g_logger) << "loadCertificates fail, cert_file="
                                            << i.cert_file << " key_file=" << i.key_file;
                }
            }
            server->setConf(i);
            // server->start();
            m_servers[i.type].push_back(server);
            svrs.push_back(server);
        }

        if (!g_service_discovery_zk->getValue().empty())
        {
            m_serviceDiscovery.reset(new ZKServiceDiscovery(g_service_discovery_zk->getValue()));
            m_rockSDLoadBalance.reset(new RockSDLoadBalance(m_serviceDiscovery));

            std::vector<TcpServer::ptr> svrs;
            if (!getServer("http", svrs))
            {
                m_serviceDiscovery->setSelfInfo(GetIPv4() + ":0:" + GetHostName());
            }
            else
            {
                std::string ip_and_port;
                for (auto &i : svrs)
                {
                    auto socks = i->getSocks();
                    for (auto &s : socks)
                    {
                        auto addr = std::dynamic_pointer_cast<IPv4Address>(s->getLocalAddress());
                        if (!addr)
                        {
                            continue;
                        }
                        auto str = addr->toString();
                        if (str.find("127.0.0.1") == 0)
                        {
                            continue;
                        }
                        if (str.find("0.0.0.0") == 0)
                        {
                            ip_and_port = GetIPv4() + ":" + std::to_string(addr->getPort());
                            break;
                        }
                        else
                        {
                            ip_and_port = addr->toString();
                        }
                    }
                    if (!ip_and_port.empty())
                    {
                        break;
                    }
                }
                m_serviceDiscovery->setSelfInfo(ip_and_port + ":" + GetHostName());
            }
        }

        for (auto &i : modules)
        {
            i->onServerReady();
        }

        for (auto &i : svrs)
        {
            i->start();
        }

        if (m_rockSDLoadBalance)
        {
            m_rockSDLoadBalance->start();
        }

        for (auto &i : modules)
        {
            i->onServerUp();
        }
        // ZKServiceDiscovery::ptr m_serviceDiscovery;
        // RockSDLoadBalance::ptr m_rockSDLoadBalance;
        // ZKServiceDiscovery::ptr zksd(new ZKServiceDiscovery("127.0.0.1:21811"));
        // zksd->registerServer("blog", "chat", GetIPv4() + ":8090", "xxx");
        // zksd->queryServer("blog", "chat");
        // zksd->setSelfInfo(GetIPv4() + ":8090");
        // zksd->setSelfData("vvv");
        // static RockSDLoadBalance::ptr rsdlb(new RockSDLoadBalance(zksd));
        // rsdlb->start();
        return 0;
    }

    bool Application::getServer(const std::string &type, std::vector<TcpServer::ptr> &svrs)
    {
        auto it = m_servers.find(type);
        if (it == m_servers.end())
        {
            return false;
        }
        svrs = it->second;
        return true;
    }

    void Application::listAllServer(std::map<std::string, std::vector<TcpServer::ptr>> &servers)
    {
        servers = m_servers;
    }

}