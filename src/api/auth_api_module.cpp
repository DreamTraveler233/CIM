#include "api/auth_api_module.hpp"

#include <jwt-cpp/jwt.h>

#include "app/auth_service.hpp"
#include "base/macro.hpp"
#include "common/common.hpp"
#include "config/config.hpp"
#include "dao/user_dao.hpp"
#include "http/http_server.hpp"
#include "http/http_servlet.hpp"
#include "system/application.hpp"
#include "util/util.hpp"

namespace CIM::api {

static auto g_logger = CIM_LOG_NAME("root");

// JWT过期时间(秒)
static auto g_jwt_expires_in =
    CIM::Config::Lookup<uint32_t>("auth.jwt.expires_in", 3600, "jwt expires in seconds");

AuthApiModule::AuthApiModule() : Module("api.auth", "0.1.0", "builtin") {}

/* 构造用户详情响应的JSON字符串 */
static std::string MakeUserDetailJson(const std::string& uid) {
    // TODO: 由用户服务从DB读取，当前返回占位数据
    Json::Value data;
    data["id"] = static_cast<Json::Int64>(std::stoll(uid.empty() ? "1" : uid));
    data["mobile"] = "18800000000";
    data["nickname"] = "demo";
    data["avatar"] = "";
    data["gender"] = 0;
    data["motto"] = "Hello, CIM";
    data["email"] = "demo@example.com";
    data["birthday"] = "1990-01-01";

    Json::Value root;
    root["code"] = 0;
    root["msg"] = "ok";
    root["data"] = data;
    return CIM::JsonUtil::ToString(root);
}

// 从请求中提取认证相关字段
static void ExtractLoginFields(CIM::http::HttpRequest::ptr req, std::string& mobile,
                               std::string& password, std::string& platform) {
    Json::Value body;
    if (ParseBody(req->getBody(), body)) {
        mobile = CIM::JsonUtil::GetString(body, "mobile", "");
        password = CIM::JsonUtil::GetString(body, "password", "");
        platform = CIM::JsonUtil::GetString(body, "platform", "web");
        if (!mobile.empty() || !password.empty()) return;
    }
}

static void ExtractPasswordUpdateFields(CIM::http::HttpRequest::ptr req, std::string& old_pwd,
                                        std::string& new_pwd) {
    Json::Value body;
    if (ParseBody(req->getBody(), body)) {
        old_pwd = CIM::JsonUtil::GetString(body, "old_password", "");
        new_pwd = CIM::JsonUtil::GetString(body, "new_password", "");
        if (!new_pwd.empty()) return;
    }
}

/* 服务器准备就绪时注册认证相关路由 */
bool AuthApiModule::onServerReady() {
    std::vector<CIM::TcpServer::ptr> httpServers;
    if (!CIM::Application::GetInstance()->getServer("http", httpServers)) {
        CIM_LOG_WARN(g_logger) << "no http servers found when registering auth routes";
        return true;
    }

    for (auto& s : httpServers) {
        auto http = std::dynamic_pointer_cast<CIM::http::HttpServer>(s);
        if (!http) continue;
        auto dispatch = http->getServletDispatch();

        /*登录接口*/
        dispatch->addServlet("/api/v1/auth/login", [](CIM::http::HttpRequest::ptr req,
                                                      CIM::http::HttpResponse::ptr res,
                                                      CIM::http::HttpSession::ptr /*session*/) {
            CIM_LOG_DEBUG(g_logger) << "/api/v1/auth/login";

            /* 设置响应头 */
            res->setHeader("Content-Type", "application/json");

            /* 提取请求字段 */
            std::string mobile, password, platform;
            Json::Value body;
            if (ParseBody(req->getBody(), body)) {
                mobile = CIM::JsonUtil::GetString(body, "mobile", "");
                password = CIM::JsonUtil::GetString(body, "password", "");
                platform = CIM::JsonUtil::GetString(body, "platform", "web");
                // 前端已经确保手机号和密码不为空，不用判断是否存在该字段
            }

            /* 鉴权用户 */
            auto Result = CIM::app::AuthService::Authenticate(mobile, password);
            if (!Result.ok) {
                res->setStatus(CIM::http::HttpStatus::UNAUTHORIZED);
                res->setBody(Error(401, Result.err));
                return 0;
            }

            /* 签发JWT */
            std::string token;
            try {
                token = SignJwt(std::to_string(Result.user.id), g_jwt_expires_in->getValue());
            } catch (const std::exception& e) {
                res->setStatus(CIM::http::HttpStatus::INTERNAL_SERVER_ERROR);
                res->setBody(Error(500, "令牌签名失败！"));
                return 0;
            }

            /* 构造并设置响应体 */
            Json::Value data;
            data["type"] = "Bearer";       // token类型，固定值Bearer
            data["access_token"] = token;  // 访问令牌
            data["expires_in"] =
                static_cast<Json::UInt>(g_jwt_expires_in->getValue());  // 过期时间(秒)
            res->setBody(Ok(data));
            return 0;
        });

        /*注册接口*/
        dispatch->addServlet("/api/v1/auth/register", [](CIM::http::HttpRequest::ptr req,
                                                         CIM::http::HttpResponse::ptr res,
                                                         CIM::http::HttpSession::ptr /*session*/) {
            CIM_LOG_DEBUG(g_logger) << "/api/v1/auth/register";
            /* 设置响应头 */
            res->setHeader("Content-Type", "application/json");

            /* 提取请求字段 */
            std::string mobile, password, platform, email, nickname;
            Json::Value body;
            if (ParseBody(req->getBody(), body)) {
                mobile = CIM::JsonUtil::GetString(body, "mobile", "");
                password = CIM::JsonUtil::GetString(body, "password", "");
                platform = CIM::JsonUtil::GetString(body, "platform", "web");
                email = CIM::JsonUtil::GetString(body, "email", "");
                nickname = CIM::JsonUtil::GetString(body, "nickname", "user");
            }

            /* 注册用户 */
            auto authResult = CIM::app::AuthService::Register(mobile, password, email, nickname);
            if (!authResult.ok) {
                res->setStatus(CIM::http::HttpStatus::BAD_REQUEST);
                res->setBody(Error(400, authResult.err));
                return 0;
            }

            /* 签发JWT */
            std::string token;
            try {
                token = SignJwt(std::to_string(authResult.user.id), g_jwt_expires_in->getValue());
            } catch (const std::exception& e) {
                res->setStatus(CIM::http::HttpStatus::INTERNAL_SERVER_ERROR);
                res->setBody(Error(500, "token sign failed"));
                return 0;
            }

            /* 设置响应体 */
            Json::Value data;
            data["type"] = "Bearer";
            data["access_token"] = token;
            data["expires_in"] = static_cast<Json::UInt>(g_jwt_expires_in->getValue());
            res->setBody(Ok(data));
            return 0;
        });

        /*找回密码接口*/
        dispatch->addServlet("/api/v1/auth/forget", [](CIM::http::HttpRequest::ptr req,
                                                       CIM::http::HttpResponse::ptr res,
                                                       CIM::http::HttpSession::ptr /*session*/) {
            CIM_LOG_DEBUG(g_logger) << "/api/v1/auth/forget";
            /* 设置响应头 */
            res->setHeader("Content-Type", "application/json");

            /*提取请求字段*/
            std::string mobile, new_password, channel;
            Json::Value body;
            if (ParseBody(req->getBody(), body)) {
                mobile = CIM::JsonUtil::GetString(body, "mobile", "");
                new_password = CIM::JsonUtil::GetString(body, "password", "");
                channel = CIM::JsonUtil::GetString(body, "channel", "");
            }

            /* 找回密码 */
            auto authResult = CIM::app::AuthService::Forget(mobile, new_password);
            if (!authResult.ok) {
                res->setStatus(CIM::http::HttpStatus::BAD_REQUEST);
                res->setBody(Error(400, authResult.err));
                return 0;
            }

            res->setBody(Ok());
            return 0;
        });

        /*获取 oauth2.0 跳转地址*/
        dispatch->addServlet("/api/v1/auth/oauth", [](CIM::http::HttpRequest::ptr /*req*/,
                                                      CIM::http::HttpResponse::ptr res,
                                                      CIM::http::HttpSession::ptr /*session*/) {
            res->setHeader("Content-Type", "application/json");
            res->setBody(Ok());
            return 0;
        });

        /*绑定第三方登录接口*/
        dispatch->addServlet(
            "/api/v1/auth/oauth/bind",
            [](CIM::http::HttpRequest::ptr /*req*/, CIM::http::HttpResponse::ptr res,
               CIM::http::HttpSession::ptr /*session*/) {
                res->setHeader("Content-Type", "application/json");
                res->setBody(Ok());
                return 0;
            });

        /*第三方登录接口*/
        dispatch->addServlet(
            "/api/v1/auth/oauth/login",
            [](CIM::http::HttpRequest::ptr /*req*/, CIM::http::HttpResponse::ptr res,
               CIM::http::HttpSession::ptr /*session*/) {
                res->setHeader("Content-Type", "application/json");
                res->setBody(Ok());
                return 0;
            });

        CIM_LOG_INFO(g_logger) << "auth routes registered";
        return true;
    }

    return true;
}

}  // namespace CIM::api
