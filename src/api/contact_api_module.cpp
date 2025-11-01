#include "api/contact_api_module.hpp"

#include "base/macro.hpp"
#include "http/http_server.hpp"
#include "http/http_servlet.hpp"
#include "system/application.hpp"
#include "util/json_util.hpp"

namespace CIM::api {
static auto g_logger = CIM_LOG_NAME("root");

ContactApiModule::ContactApiModule() : Module("api.contact", "0.1.0", "builtin") {}

static std::string Ok(const Json::Value& data = Json::Value(Json::objectValue)) {
    Json::Value root;
    root["code"] = 0;
    root["msg"] = "ok";
    root["data"] = data;
    return CIM::JsonUtil::ToString(root);
}

bool ContactApiModule::onServerReady() {
    std::vector<CIM::TcpServer::ptr> httpServers;
    if (!CIM::Application::GetInstance()->getServer("http", httpServers)) {
        CIM_LOG_WARN(g_logger) << "no http servers found when registering contact routes";
        return true;
    }

    for (auto& s : httpServers) {
        auto http = std::dynamic_pointer_cast<CIM::http::HttpServer>(s);
        if (!http) continue;
        auto dispatch = http->getServletDispatch();

        // contact-apply
        dispatch->addServlet(
            "/api/v1/contact-apply/accept",
            [](CIM::http::HttpRequest::ptr /*req*/, CIM::http::HttpResponse::ptr res,
               CIM::http::HttpSession::ptr /*session*/) {
                res->setHeader("Content-Type", "application/json");
                res->setBody(Ok());
                return 0;
            });
        dispatch->addServlet(
            "/api/v1/contact-apply/create",
            [](CIM::http::HttpRequest::ptr /*req*/, CIM::http::HttpResponse::ptr res,
               CIM::http::HttpSession::ptr /*session*/) {
                res->setHeader("Content-Type", "application/json");
                res->setBody(Ok());
                return 0;
            });
        dispatch->addServlet(
            "/api/v1/contact-apply/decline",
            [](CIM::http::HttpRequest::ptr /*req*/, CIM::http::HttpResponse::ptr res,
               CIM::http::HttpSession::ptr /*session*/) {
                res->setHeader("Content-Type", "application/json");
                res->setBody(Ok());
                return 0;
            });
        dispatch->addServlet(
            "/api/v1/contact-apply/list",
            [](CIM::http::HttpRequest::ptr /*req*/, CIM::http::HttpResponse::ptr res,
               CIM::http::HttpSession::ptr /*session*/) {
                res->setHeader("Content-Type", "application/json");
                Json::Value d;
                d["list"] = Json::Value(Json::arrayValue);
                res->setBody(Ok(d));
                return 0;
            });
        dispatch->addServlet(
            "/api/v1/contact-apply/unread-num",
            [](CIM::http::HttpRequest::ptr /*req*/, CIM::http::HttpResponse::ptr res,
               CIM::http::HttpSession::ptr /*session*/) {
                res->setHeader("Content-Type", "application/json");
                Json::Value d;
                d["count"] = 0;
                res->setBody(Ok(d));
                return 0;
            });

        // contact-group
        dispatch->addServlet(
            "/api/v1/contact-group/list",
            [](CIM::http::HttpRequest::ptr /*req*/, CIM::http::HttpResponse::ptr res,
               CIM::http::HttpSession::ptr /*session*/) {
                res->setHeader("Content-Type", "application/json");
                Json::Value d;
                d["list"] = Json::Value(Json::arrayValue);
                res->setBody(Ok(d));
                return 0;
            });
        dispatch->addServlet(
            "/api/v1/contact-group/save",
            [](CIM::http::HttpRequest::ptr /*req*/, CIM::http::HttpResponse::ptr res,
               CIM::http::HttpSession::ptr /*session*/) {
                res->setHeader("Content-Type", "application/json");
                res->setBody(Ok());
                return 0;
            });

        // contact
        dispatch->addServlet(
            "/api/v1/contact/change-group",
            [](CIM::http::HttpRequest::ptr /*req*/, CIM::http::HttpResponse::ptr res,
               CIM::http::HttpSession::ptr /*session*/) {
                res->setHeader("Content-Type", "application/json");
                res->setBody(Ok());
                return 0;
            });
        dispatch->addServlet("/api/v1/contact/delete", [](CIM::http::HttpRequest::ptr /*req*/,
                                                          CIM::http::HttpResponse::ptr res,
                                                          CIM::http::HttpSession::ptr /*session*/) {
            res->setHeader("Content-Type", "application/json");
            res->setBody(Ok());
            return 0;
        });
        dispatch->addServlet("/api/v1/contact/detail", [](CIM::http::HttpRequest::ptr /*req*/,
                                                          CIM::http::HttpResponse::ptr res,
                                                          CIM::http::HttpSession::ptr /*session*/) {
            res->setHeader("Content-Type", "application/json");
            Json::Value d(Json::objectValue);
            res->setBody(Ok(d));
            return 0;
        });
        dispatch->addServlet(
            "/api/v1/contact/edit-remark",
            [](CIM::http::HttpRequest::ptr /*req*/, CIM::http::HttpResponse::ptr res,
               CIM::http::HttpSession::ptr /*session*/) {
                res->setHeader("Content-Type", "application/json");
                res->setBody(Ok());
                return 0;
            });
        // 注意：/api/v1/contact/list 已由 ImApiModule 提供占位；如需迁移，请移除Im中的重复注册
        dispatch->addServlet(
            "/api/v1/contact/online-status",
            [](CIM::http::HttpRequest::ptr /*req*/, CIM::http::HttpResponse::ptr res,
               CIM::http::HttpSession::ptr /*session*/) {
                res->setHeader("Content-Type", "application/json");
                Json::Value d;
                d["list"] = Json::Value(Json::arrayValue);
                res->setBody(Ok(d));
                return 0;
            });
        dispatch->addServlet("/api/v1/contact/search", [](CIM::http::HttpRequest::ptr /*req*/,
                                                          CIM::http::HttpResponse::ptr res,
                                                          CIM::http::HttpSession::ptr /*session*/) {
            res->setHeader("Content-Type", "application/json");
            Json::Value d;
            d["list"] = Json::Value(Json::arrayValue);
            res->setBody(Ok(d));
            return 0;
        });
    }

    CIM_LOG_INFO(g_logger) << "contact routes registered";
    return true;
}

}  // namespace CIM::api
