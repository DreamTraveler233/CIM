#include "api/article_api_module.hpp"

#include "base/macro.hpp"
#include "http/http_server.hpp"
#include "http/http_servlet.hpp"
#include "system/application.hpp"
#include "util/json_util.hpp"

namespace CIM::api {
static auto g_logger = CIM_LOG_NAME("root");

ArticleApiModule::ArticleApiModule() : Module("api.article", "0.1.0", "builtin") {}

static std::string Ok(const Json::Value& data = Json::Value(Json::objectValue)) {
    Json::Value root;
    root["code"] = 0;
    root["msg"] = "ok";
    root["data"] = data;
    return CIM::JsonUtil::ToString(root);
}

bool ArticleApiModule::onServerReady() {
    std::vector<CIM::TcpServer::ptr> httpServers;
    if (!CIM::Application::GetInstance()->getServer("http", httpServers)) {
        CIM_LOG_WARN(g_logger) << "no http servers found when registering article routes";
        return true;
    }

    for (auto& s : httpServers) {
        auto http = std::dynamic_pointer_cast<CIM::http::HttpServer>(s);
        if (!http) continue;
        auto dispatch = http->getServletDispatch();

        // article-annex
        dispatch->addServlet("/api/v1/article-annex/delete",
                             [](CIM::http::HttpRequest::ptr, CIM::http::HttpResponse::ptr res,
                                CIM::http::HttpSession::ptr) {
                                 res->setHeader("Content-Type", "application/json");
                                 res->setBody(Ok());
                                 return 0;
                             });
        dispatch->addServlet("/api/v1/article-annex/forever-delete",
                             [](CIM::http::HttpRequest::ptr, CIM::http::HttpResponse::ptr res,
                                CIM::http::HttpSession::ptr) {
                                 res->setHeader("Content-Type", "application/json");
                                 res->setBody(Ok());
                                 return 0;
                             });
        dispatch->addServlet("/api/v1/article-annex/recover",
                             [](CIM::http::HttpRequest::ptr, CIM::http::HttpResponse::ptr res,
                                CIM::http::HttpSession::ptr) {
                                 res->setHeader("Content-Type", "application/json");
                                 res->setBody(Ok());
                                 return 0;
                             });
        dispatch->addServlet("/api/v1/article-annex/recover-list",
                             [](CIM::http::HttpRequest::ptr, CIM::http::HttpResponse::ptr res,
                                CIM::http::HttpSession::ptr) {
                                 res->setHeader("Content-Type", "application/json");
                                 Json::Value d;
                                 d["list"] = Json::Value(Json::arrayValue);
                                 res->setBody(Ok(d));
                                 return 0;
                             });

        // article classify
        dispatch->addServlet("/api/v1/article/classify/delete",
                             [](CIM::http::HttpRequest::ptr, CIM::http::HttpResponse::ptr res,
                                CIM::http::HttpSession::ptr) {
                                 res->setHeader("Content-Type", "application/json");
                                 res->setBody(Ok());
                                 return 0;
                             });
        dispatch->addServlet("/api/v1/article/classify/edit",
                             [](CIM::http::HttpRequest::ptr, CIM::http::HttpResponse::ptr res,
                                CIM::http::HttpSession::ptr) {
                                 res->setHeader("Content-Type", "application/json");
                                 res->setBody(Ok());
                                 return 0;
                             });
        dispatch->addServlet("/api/v1/article/classify/list",
                             [](CIM::http::HttpRequest::ptr, CIM::http::HttpResponse::ptr res,
                                CIM::http::HttpSession::ptr) {
                                 res->setHeader("Content-Type", "application/json");
                                 Json::Value d;
                                 d["list"] = Json::Value(Json::arrayValue);
                                 res->setBody(Ok(d));
                                 return 0;
                             });
        dispatch->addServlet("/api/v1/article/classify/sort",
                             [](CIM::http::HttpRequest::ptr, CIM::http::HttpResponse::ptr res,
                                CIM::http::HttpSession::ptr) {
                                 res->setHeader("Content-Type", "application/json");
                                 res->setBody(Ok());
                                 return 0;
                             });

        // article core
        dispatch->addServlet("/api/v1/article/delete",
                             [](CIM::http::HttpRequest::ptr, CIM::http::HttpResponse::ptr res,
                                CIM::http::HttpSession::ptr) {
                                 res->setHeader("Content-Type", "application/json");
                                 res->setBody(Ok());
                                 return 0;
                             });
        dispatch->addServlet("/api/v1/article/detail",
                             [](CIM::http::HttpRequest::ptr, CIM::http::HttpResponse::ptr res,
                                CIM::http::HttpSession::ptr) {
                                 res->setHeader("Content-Type", "application/json");
                                 Json::Value d(Json::objectValue);
                                 res->setBody(Ok(d));
                                 return 0;
                             });
        dispatch->addServlet("/api/v1/article/editor",
                             [](CIM::http::HttpRequest::ptr, CIM::http::HttpResponse::ptr res,
                                CIM::http::HttpSession::ptr) {
                                 res->setHeader("Content-Type", "application/json");
                                 res->setBody(Ok());
                                 return 0;
                             });
        dispatch->addServlet("/api/v1/article/forever-delete",
                             [](CIM::http::HttpRequest::ptr, CIM::http::HttpResponse::ptr res,
                                CIM::http::HttpSession::ptr) {
                                 res->setHeader("Content-Type", "application/json");
                                 res->setBody(Ok());
                                 return 0;
                             });
        dispatch->addServlet("/api/v1/article/list",
                             [](CIM::http::HttpRequest::ptr, CIM::http::HttpResponse::ptr res,
                                CIM::http::HttpSession::ptr) {
                                 res->setHeader("Content-Type", "application/json");
                                 Json::Value d;
                                 d["list"] = Json::Value(Json::arrayValue);
                                 res->setBody(Ok(d));
                                 return 0;
                             });
        dispatch->addServlet("/api/v1/article/move",
                             [](CIM::http::HttpRequest::ptr, CIM::http::HttpResponse::ptr res,
                                CIM::http::HttpSession::ptr) {
                                 res->setHeader("Content-Type", "application/json");
                                 res->setBody(Ok());
                                 return 0;
                             });
        dispatch->addServlet("/api/v1/article/recover",
                             [](CIM::http::HttpRequest::ptr, CIM::http::HttpResponse::ptr res,
                                CIM::http::HttpSession::ptr) {
                                 res->setHeader("Content-Type", "application/json");
                                 res->setBody(Ok());
                                 return 0;
                             });
        dispatch->addServlet("/api/v1/article/recover-list",
                             [](CIM::http::HttpRequest::ptr, CIM::http::HttpResponse::ptr res,
                                CIM::http::HttpSession::ptr) {
                                 res->setHeader("Content-Type", "application/json");
                                 Json::Value d;
                                 d["list"] = Json::Value(Json::arrayValue);
                                 res->setBody(Ok(d));
                                 return 0;
                             });
        dispatch->addServlet("/api/v1/article/tags",
                             [](CIM::http::HttpRequest::ptr, CIM::http::HttpResponse::ptr res,
                                CIM::http::HttpSession::ptr) {
                                 res->setHeader("Content-Type", "application/json");
                                 res->setBody(Ok());
                                 return 0;
                             });
        dispatch->addServlet("/api/v1/article/asterisk",
                             [](CIM::http::HttpRequest::ptr, CIM::http::HttpResponse::ptr res,
                                CIM::http::HttpSession::ptr) {
                                 res->setHeader("Content-Type", "application/json");
                                 res->setBody(Ok());
                                 return 0;
                             });
    }

    CIM_LOG_INFO(g_logger) << "article routes registered";
    return true;
}

}  // namespace CIM::api
