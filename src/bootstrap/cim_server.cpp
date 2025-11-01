#include "api/auth_api_module.hpp"
#include "api/common_api_module.hpp"
#include "api/article_api_module.hpp"
#include "api/contact_api_module.hpp"
#include "api/emoticon_api_module.hpp"
#include "api/group_api_module.hpp"
#include "api/message_api_module.hpp"
#include "api/organize_api_module.hpp"
#include "api/talk_api_module.hpp"
#include "api/user_api_module.hpp"
#include "api/minimal_api_module.hpp"
#include "api/im_api_module.hpp"
#include "api/ws_gateway_module.hpp"
#include "base/macro.hpp"
#include "common/config_loader.hpp"
#include "http/http_server.hpp"
#include "other/crypto_module.hpp"
#include "other/module.hpp"
#include "system/application.hpp"

using namespace CIM;

int main(int argc, char** argv) {
    /* 创建并初始化应用程序实例 */
    Application app;
    if (!app.init(argc, argv)) {
        CIM_LOG_ERROR(CIM_LOG_ROOT()) << "Application init failed";
        return 1;
    }

    // 打印一次 servers 配置摘要，便于启动时确认配置生效
    CIM::common::LogServerConfigSummary();

    // 注册加解密模块（优先），然后注册最小 API 模块 & 鉴权模块
    CIM::ModuleMgr::GetInstance()->add(std::make_shared<CIM::CryptoModule>());           // 注册加解密模块
    CIM::ModuleMgr::GetInstance()->add(std::make_shared<CIM::api::AuthApiModule>());     // 注册登录鉴权模块
    CIM::ModuleMgr::GetInstance()->add(std::make_shared<CIM::api::CommonApiModule>());   // 注册通用 API 模块
    // 业务占位模块
    CIM::ModuleMgr::GetInstance()->add(std::make_shared<CIM::api::ArticleApiModule>());  // 文章模块占位
    CIM::ModuleMgr::GetInstance()->add(std::make_shared<CIM::api::ContactApiModule>());  // 联系人模块占位
    CIM::ModuleMgr::GetInstance()->add(std::make_shared<CIM::api::EmoticonApiModule>()); // 表情模块占位
    CIM::ModuleMgr::GetInstance()->add(std::make_shared<CIM::api::GroupApiModule>());    // 群组模块占位
    CIM::ModuleMgr::GetInstance()->add(std::make_shared<CIM::api::MessageApiModule>());  // 消息模块占位
    CIM::ModuleMgr::GetInstance()->add(std::make_shared<CIM::api::OrganizeApiModule>()); // 组织架构模块占位
    CIM::ModuleMgr::GetInstance()->add(std::make_shared<CIM::api::TalkApiModule>());     // 会话模块占位
    CIM::ModuleMgr::GetInstance()->add(std::make_shared<CIM::api::UserApiModule>());     // 用户模块占位
    CIM::ModuleMgr::GetInstance()->add(std::make_shared<CIM::api::ImApiModule>());       // IM 全局占位（仅用户设置）
    CIM::ModuleMgr::GetInstance()->add(std::make_shared<CIM::api::WsGatewayModule>());   // WebSocket 网关模块

    return app.run() ? 0 : 2;
}
