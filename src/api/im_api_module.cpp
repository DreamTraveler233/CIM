#include "api/im_api_module.hpp"

#include "base/macro.hpp"
#include "http/http_server.hpp"
#include "http/http_servlet.hpp"
#include "system/application.hpp"
#include "util/json_util.hpp"

namespace CIM::api {
static auto g_logger = CIM_LOG_NAME("root");

ImApiModule::ImApiModule() : Module("api.im", "0.1.0", "builtin") {}

// 统一成功响应封装
static std::string Ok(const Json::Value& data = Json::Value(Json::objectValue)) {
    Json::Value root;
    root["code"] = 0;
    root["msg"] = "ok";
    root["data"] = data;
    return CIM::JsonUtil::ToString(root);
}

bool ImApiModule::onServerReady() {
    std::vector<CIM::TcpServer::ptr> httpServers;
    if (!CIM::Application::GetInstance()->getServer("http", httpServers)) {
        CIM_LOG_WARN(g_logger) << "no http servers found when registering im routes";
        return true;
    }

    for (auto& s : httpServers) {
        auto http = std::dynamic_pointer_cast<CIM::http::HttpServer>(s);
        if (!http) continue;
        auto dispatch = http->getServletDispatch();

        // 其余 IM 相关接口分散在具体模块中（contact/group/talk等），避免重复注册
    }

    CIM_LOG_INFO(g_logger)
        << "im routes registered: /api/v1/user/setting, /api/v1/contact-apply/unread-num, "
           "/api/v1/group-apply/unread-num, /api/v1/talk/session-list, /api/v1/contact/list, "
           "/api/v1/contact-group/list";
    return true;
}

}  // namespace CIM::api
