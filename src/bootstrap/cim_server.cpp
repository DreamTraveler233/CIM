#include "system/application.hpp"
#include "http/http_server.hpp"
#include "base/macro.hpp"
#include "other/module.hpp"
#include "api/minimal_api_module.hpp"

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

    // 注册最小 API 模块，路由会在 onServerReady 钩子中注入
    CIM::ModuleMgr::GetInstance()->add(std::make_shared<CIM::api::MinimalApiModule>());

    return app.run() ? 0 : 2;
}
