#include "common/config_loader.hpp"

#include "config.hpp"
#include "macro.hpp"
#include "net/tcp_server.hpp"

namespace CIM::common
{

static auto g_logger = CIM_LOG_NAME("system");

// 与 application.cpp 中相同的键名，重新声明一个本地的 Config 句柄来读取
static auto g_servers_conf = CIM::Config::Lookup(
    "servers", std::vector<CIM::TcpServerConf>{}, "http server config");

void LogServerConfigSummary()
{
    const auto& svrs = g_servers_conf->getValue();
    if (svrs.empty())
    {
        CIM_LOG_WARN(g_logger) << "servers config is empty";
        return;
    }

    CIM_LOG_INFO(g_logger) << "servers count=" << svrs.size();
    for (const auto& c : svrs)
    {
        std::string addrs;
        for (size_t i = 0; i < c.address.size(); ++i)
        {
            addrs += c.address[i];
            if (i + 1 < c.address.size()) addrs += ",";
        }
        CIM_LOG_INFO(g_logger)
            << "type=" << c.type << " name=" << c.name
            << " keepalive=" << c.keepalive << " timeout_ms=" << c.timeout
            << " accept_worker=" << c.accept_worker
            << " io_worker=" << c.io_worker
            << " process_worker=" << c.process_worker << " address=[" << addrs
            << "]";
    }
}

}  // namespace CIM::common
