#include "api/auth_api_module.hpp"
#include "api/common_api_module.hpp"
#include "api/minimal_api_module.hpp"
#include "other/crypto_module.hpp"
#include "base/macro.hpp"
#include "common/config_loader.hpp"
#include "http/http_server.hpp"
#include "other/module.hpp"
#include "system/application.hpp"

using namespace CIM;

int main(int argc, char **argv)
{
    /* 创建并初始化应用程序实例 */
    Application app;
    if (!app.init(argc, argv))
    {
        CIM_LOG_ERROR(CIM_LOG_ROOT()) << "Application init failed";
        return 1;
    }

    // 打印一次 servers 配置摘要，便于启动时确认配置生效
    CIM::common::LogServerConfigSummary();

    // 注册加解密模块（优先），然后注册最小 API 模块 & 鉴权模块
    CIM::ModuleMgr::GetInstance()->add(std::make_shared<CIM::CryptoModule>());
    CIM::ModuleMgr::GetInstance()->add(std::make_shared<CIM::api::AuthApiModule>());
    CIM::ModuleMgr::GetInstance()->add(std::make_shared<CIM::api::CommonApiModule>());

    return app.run() ? 0 : 2;
}
