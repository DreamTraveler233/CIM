#include <gtest/gtest.h>
#include "config.hpp"
#include "net/tcp_server.hpp"
#include "common/config_loader.hpp"

// 目标：验证配置加载中 servers 段至少存在一项，并且汇总函数可安全调用。
// 依赖 bin/config/server.yaml，请确保构建后运行于项目 bin/ 目录。

TEST(ConfigSummary, ServersExistAndLog) {
    // 按 application 中的约定键名读取配置
    auto servers_conf = CIM::Config::Lookup("servers", std::vector<CIM::TcpServerConf>{}, "http server config");

    // 不做过强约束，但一般应 >=1（http/ws 至少一个）。
    // 若你的环境暂无配置，可将阈值改为 >=0。
    EXPECT_GE(static_cast<int>(servers_conf->getValue().size()), 1);

    // 调用日志输出方法，验证不崩溃
    ASSERT_NO_THROW({ CIM::common::LogServerConfigSummary(); });
}
