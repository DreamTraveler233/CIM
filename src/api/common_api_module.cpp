#include "api/common_api_module.hpp"

#include "base/macro.hpp"
#include "db/mysql.hpp"
#include "http/http_server.hpp"
#include "http/http_servlet.hpp"
#include "system/application.hpp"
#include "util/util.hpp"
#include "app/common_service.hpp"

namespace CIM::api {
static auto g_logger = CIM_LOG_NAME("root");

CommonApiModule::CommonApiModule() : Module("common", "0.1.0", "builtin") {}

static std::string MakeResponseJson(const Json::Value& data) {
    return CIM::JsonUtil::ToString(data);
}

static std::string MakeErrorJson(int code, const std::string& msg) {
    Json::Value root;
    root["code"] = code;
    root["msg"] = msg;
    return CIM::JsonUtil::ToString(root);
}

// 解析 JSON 请求体
static bool ParseJsonBody(const std::string& body, Json::Value& out) {
    if (body.empty()) return false;
    if (!CIM::JsonUtil::FromString(out, body)) {
        return false;
    }
    return out.isObject();
}

// 提取短信请求字段：mobile, channel
static void ExtractSmsFields(CIM::http::HttpRequest::ptr req, std::string& mobile, std::string& channel) {
    Json::Value body;
    if (ParseJsonBody(req->getBody(), body)) {
        mobile = CIM::JsonUtil::GetString(body, "mobile", mobile);
        channel = CIM::JsonUtil::GetString(body, "channel", channel);
        if (!mobile.empty()) {
            return;
        }
    }
}

bool CommonApiModule::onServerReady() {
    std::vector<CIM::TcpServer::ptr> httpServers;
    if (!CIM::Application::GetInstance()->getServer("http", httpServers)) {
        CIM_LOG_WARN(g_logger) << "no http servers found when registering common routes";
        return true;
    }

    for (auto& s : httpServers) {
        auto http = std::dynamic_pointer_cast<CIM::http::HttpServer>(s);
        if (!http) continue;
        auto dispatch = http->getServletDispatch();

        // 发送短信验证码
        dispatch->addServlet("/api/v1/common/send-sms",
                             [](CIM::http::HttpRequest::ptr req, CIM::http::HttpResponse::ptr res,
                                CIM::http::HttpSession::ptr /*session*/) {
                                 CIM_LOG_DEBUG(g_logger) << "send-sms request body: " << std::endl << req->getBody();

                                 res->setHeader("Content-Type", "application/json");

                                 // 提取字段，channel字段前端默认为register
                                 std::string mobile, channel;
                                 ExtractSmsFields(req, mobile, channel);
                                 if (mobile.empty()) {
                                     res->setStatus(CIM::http::HttpStatus::BAD_REQUEST);
                                     res->setBody(MakeErrorJson(422, "mobile required"));
                                     return 0;
                                 }

                                // 生成验证码
                                std::string sms_code = CIM::app::CommonService::SendSmsCode();

                                 // TODO: 集成实际短信网关（阿里云/腾讯云/云片等），当前返回验证码用于开发联调

                                 /* 构造并设置响应体 */
                                 Json::Value data;
                                 data["sms_code"] = sms_code;
                                 res->setBody(CIM::JsonUtil::ToString(data));
                                 return 0;
                             });

        // 注册邮箱服务（占位实现）
        dispatch->addServlet("/api/v1/common/send-email",
                             [](CIM::http::HttpRequest::ptr req, CIM::http::HttpResponse::ptr res,
                                CIM::http::HttpSession::ptr /*session*/) {
                                 res->setHeader("Content-Type", "application/json");
                                 res->setBody("{\"code\":0,\"msg\":\"ok\",\"data\":{\"status\":\"running\"}} ");
                                 return 0;
                             });
    }

    CIM_LOG_INFO(g_logger) << "common routes registered: /api/v1/common/send-sms, "
                              "/api/v1/common/send-email";
    return true;
}

}  // namespace CIM::api
