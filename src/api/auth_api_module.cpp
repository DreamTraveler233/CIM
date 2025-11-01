#include "api/auth_api_module.hpp"

#include <jwt-cpp/jwt.h>

#include "app/auth_service.hpp"
#include "base/macro.hpp"
#include "config/config.hpp"
#include "dao/user_dao.hpp"
#include "http/http_server.hpp"
#include "http/http_servlet.hpp"
#include "system/application.hpp"
#include "util/string_util.hpp"
#include "util/util.hpp"

namespace CIM::api {

static auto g_logger = CIM_LOG_NAME("root");

// JWT签名密钥
static auto g_jwt_secret = CIM::Config::Lookup<std::string>(
    "auth.jwt.secret", std::string("dev-secret"), "jwt hmac secret");
// JWT签发者
static auto g_jwt_issuer =
    CIM::Config::Lookup<std::string>("auth.jwt.issuer", std::string("auth-service"), "jwt issuer");
// JWT过期时间(秒)
static auto g_jwt_expires_in =
    CIM::Config::Lookup<uint32_t>("auth.jwt.expires_in", 3600, "jwt expires in seconds");

AuthApiModule::AuthApiModule() : Module("api.auth", "0.1.0", "builtin") {}

/* 构造错误响应的JSON字符串 */
static std::string MakeErrorJson(int code, const std::string& msg) {
    Json::Value root;
    root["code"] = code;
    // 为了兼容前端ApiClient错误处理，统一使用`message`字段
    root["message"] = msg;
    return CIM::JsonUtil::ToString(root);
}

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

/* 解析请求体中的JSON字符串 */
static bool ParseJsonBody(const std::string& body, Json::Value& out) {
    if (body.empty()) return false;
    if (!CIM::JsonUtil::FromString(out, body)) return false;
    return out.isObject();
}

// 从请求中提取认证相关字段
static void ExtractAuthFields(CIM::http::HttpRequest::ptr req, std::string& mobile,
                              std::string& password, std::string& platform, std::string& email,
                              std::string& nickname) {
    Json::Value body;
    if (ParseJsonBody(req->getBody(), body)) {
        mobile = CIM::JsonUtil::GetString(body, "mobile", "");
        password = CIM::JsonUtil::GetString(body, "password", "");
        platform = CIM::JsonUtil::GetString(body, "platform", "web");
        email = CIM::JsonUtil::GetString(body, "email", "");
        nickname = CIM::JsonUtil::GetString(body, "nickname", "");
        if (!mobile.empty() || !password.empty()) return;
    }
}

static void ExtractPasswordUpdateFields(CIM::http::HttpRequest::ptr req, std::string& old_pwd,
                                        std::string& new_pwd) {
    Json::Value body;
    if (ParseJsonBody(req->getBody(), body)) {
        old_pwd = CIM::JsonUtil::GetString(body, "old_password", "");
        new_pwd = CIM::JsonUtil::GetString(body, "new_password", "");
        if (!new_pwd.empty()) return;
    }
}

/**
     * @brief 使用HS256算法签发JWT令牌
     * @param uid 用户ID，将作为主题和载荷声明添加到JWT中
     * @param expires_in JWT令牌有效期（秒）
     * @return 签名后的JWT令牌字符串
     */
static std::string SignJwt(const std::string& uid, uint32_t expires_in) {
    auto now = std::chrono::system_clock::now();
    auto exp = now + std::chrono::seconds(expires_in);
    return jwt::create()
        .set_type("JWS")
        .set_issuer(g_jwt_issuer->getValue())
        .set_issued_at(now)
        .set_expires_at(exp)
        .set_subject(uid)
        .set_payload_claim("uid", jwt::claim(uid))
        .sign(jwt::algorithm::hs256{g_jwt_secret->getValue()});
}

/**
     * @brief 验证JWT令牌的有效性
     * @param token[in] 待验证的JWT令牌字符串
     * @param out_uid[out] 可选输出参数，用于返回用户ID
     * @return 验证成功返回true，失败返回false
     *
     * 该函数使用HS256算法和预设的密钥及发行者信息验证JWT令牌。
     * 如果验证成功且提供了out_uid指针，则尝试从令牌中提取用户ID。
     * 用户ID优先从subject字段获取，如果失败则尝试从自定义的"uid"声明中获取。
     */
static bool VerifyJwt(const std::string& token, std::string* out_uid = nullptr) {
    try {
        // 解析并验证JWT令牌
        auto dec = jwt::decode(token);
        auto verifier = jwt::verify()
                            .allow_algorithm(jwt::algorithm::hs256{g_jwt_secret->getValue()})
                            .with_issuer(g_jwt_issuer->getValue());
        verifier.verify(dec);

        // 如果需要输出用户ID，则从令牌中提取
        if (out_uid) {
            try {
                // 首先尝试从标准subject字段获取用户ID
                auto sub = dec.get_subject();
                *out_uid = sub;
            } catch (...) {
                // 尝试从自定义claim读取
                auto c = dec.get_payload_claim("uid");
                *out_uid = c.as_string();
            }
        }
        return true;
    } catch (const std::exception& e) {
        CIM_LOG_WARN(g_logger) << "jwt verify failed: " << e.what();
        return false;
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

        // 登录：读取 JSON/FORM 体，校验基础字段并签发HS256 JWT
        dispatch->addServlet("/api/v1/auth/login", [](CIM::http::HttpRequest::ptr req,
                                                      CIM::http::HttpResponse::ptr res,
                                                      CIM::http::HttpSession::ptr /*session*/) {
            CIM_LOG_DEBUG(g_logger) << "register request body: " << std::endl << req->getBody();

            /* 设置响应头 */
            res->setHeader("Content-Type", "application/json");

            /* 提取请求字段 */
            std::string mobile, password, platform, email, nickname;
            ExtractAuthFields(req, mobile, password, platform, email, nickname);
            if (mobile.empty() || password.empty()) {
                res->setStatus(CIM::http::HttpStatus::BAD_REQUEST);
                res->setBody(MakeErrorJson(422, "mobile/password required"));
                return 0;
            }

            /* 鉴权用户 */
            auto authResult = CIM::app::AuthService::Authenticate(mobile, password);
            if (!authResult.ok) {
                res->setStatus(CIM::http::HttpStatus::UNAUTHORIZED);
                res->setBody(MakeErrorJson(401, authResult.err));
                return 0;
            }

            /* 签发JWT */
            std::string token;
            try {
                token = SignJwt(std::to_string(authResult.user.id), g_jwt_expires_in->getValue());
            } catch (const std::exception& e) {
                res->setStatus(CIM::http::HttpStatus::INTERNAL_SERVER_ERROR);
                res->setBody(MakeErrorJson(500, "token sign failed"));
                return 0;
            }

            /* 构造并设置响应体 */
            Json::Value data;
            data["type"] = "Bearer";
            data["access_token"] = token;
            data["expires_in"] = static_cast<Json::UInt>(g_jwt_expires_in->getValue());
            res->setBody(CIM::JsonUtil::ToString(data));
            return 0;
        });

        // 注册：写入DB并回传已登录的 token
        dispatch->addServlet("/api/v1/auth/register", [](CIM::http::HttpRequest::ptr req,
                                                         CIM::http::HttpResponse::ptr res,
                                                         CIM::http::HttpSession::ptr /*session*/) {
            /* 设置响应头 */
            res->setHeader("Content-Type", "application/json");

            /* 提取请求字段 */
            std::string mobile, password, platform, email, nickname;
            ExtractAuthFields(req, mobile, password, platform, email, nickname);
            if (mobile.empty() || password.empty()) {
                res->setStatus(CIM::http::HttpStatus::BAD_REQUEST);
                res->setBody(MakeErrorJson(422, "mobile/password required"));
                return 0;
            }

            /* 注册用户 */
            auto authResult = CIM::app::AuthService::Register(mobile, password, email, nickname);
            if (!authResult.ok) {
                res->setStatus(CIM::http::HttpStatus::BAD_REQUEST);
                res->setBody(MakeErrorJson(400, authResult.err));
                return 0;
            }

            /* 签发JWT */
            std::string token;
            try {
                token = SignJwt(std::to_string(authResult.user.id), g_jwt_expires_in->getValue());
            } catch (const std::exception& e) {
                res->setStatus(CIM::http::HttpStatus::INTERNAL_SERVER_ERROR);
                res->setBody(MakeErrorJson(500, "token sign failed"));
                return 0;
            }

            /* 设置响应体 */
            Json::Value data;
            data["type"] = "Bearer";
            data["access_token"] = token;
            data["expires_in"] = static_cast<Json::UInt>(g_jwt_expires_in->getValue());
            res->setBody(CIM::JsonUtil::ToString(data));
            return 0;
        });

    CIM_LOG_INFO(g_logger) << "auth routes registered: /api/v1/auth/login, "
                              "/api/v1/auth/register";
    return true;
}

}  // namespace CIM::api
