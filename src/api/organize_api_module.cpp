#include "api/organize_api_module.hpp"

#include "base/macro.hpp"
#include "http/http_server.hpp"
#include "http/http_servlet.hpp"
#include "system/application.hpp"
#include "util/json_util.hpp"

namespace CIM::api {
static auto g_logger = CIM_LOG_NAME("root");

OrganizeApiModule::OrganizeApiModule() : Module("api.organize", "0.1.0", "builtin") {}

static std::string Ok(const Json::Value& data = Json::Value(Json::objectValue)) {
    Json::Value root;
    root["code"] = 0;
    root["msg"] = "ok";
    root["data"] = data;
    return CIM::JsonUtil::ToString(root);
}

bool OrganizeApiModule::onServerReady() {
    std::vector<CIM::TcpServer::ptr> httpServers;
    if (!CIM::Application::GetInstance()->getServer("http", httpServers)) {
        CIM_LOG_WARN(g_logger) << "no http servers found when registering organize routes";
        return true;
    }

    for (auto& s : httpServers) {
        auto http = std::dynamic_pointer_cast<CIM::http::HttpServer>(s);
        if (!http) continue;
        auto dispatch = http->getServletDispatch();

        dispatch->addServlet("/api/v1/organize/department-list",
                             [](CIM::http::HttpRequest::ptr, CIM::http::HttpResponse::ptr res,
                                CIM::http::HttpSession::ptr) {
                                 res->setHeader("Content-Type", "application/json");
                                 Json::Value d;
                                 d["list"] = Json::Value(Json::arrayValue);
                                 res->setBody(Ok(d));
                                 return 0;
                             });
        dispatch->addServlet("/api/v1/organize/personnel-list",
                             [](CIM::http::HttpRequest::ptr, CIM::http::HttpResponse::ptr res,
                                CIM::http::HttpSession::ptr) {
                                 res->setHeader("Content-Type", "application/json");
                                 Json::Value d;
                                 d["list"] = Json::Value(Json::arrayValue);
                                 res->setBody(Ok(d));
                                 return 0;
                             });
    }

    CIM_LOG_INFO(g_logger) << "organize routes registered";
    return true;
}

}  // namespace CIM::api
