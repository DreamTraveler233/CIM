#include "common/common.hpp"

#include <jwt-cpp/jwt.h>

#include "config.hpp"
#include "macro.hpp"
#include "net/tcp_server.hpp"
#include "util/json_util.hpp"

namespace CIM {

static auto g_logger = CIM_LOG_NAME("system");

// JWT签名密钥
static auto g_jwt_secret = CIM::Config::Lookup<std::string>(
    "auth.jwt.secret", std::string("dev-secret"), "jwt hmac secret");
// JWT签发者
static auto g_jwt_issuer =
    CIM::Config::Lookup<std::string>("auth.jwt.issuer", std::string("auth-service"), "jwt issuer");

std::string Ok(const Json::Value& data) {
    return CIM::JsonUtil::ToString(data);
}

std::string Error(int code, const std::string& msg) {
    Json::Value root;
    root["code"] = code;
    root["message"] = msg;
    return CIM::JsonUtil::ToString(root);
}

bool ParseBody(const std::string& body, Json::Value& out) {
    if (body.empty()) return false;
    if (!CIM::JsonUtil::FromString(out, body)) return false;
    return out.isObject();
}

std::string SignJwt(const std::string& uid, uint32_t expires_in) {
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

bool VerifyJwt(const std::string& token, std::string* out_uid) {
    try {
        auto dec = jwt::decode(token);
        auto verifier = jwt::verify()
                            .allow_algorithm(jwt::algorithm::hs256{g_jwt_secret->getValue()})
                            .with_issuer(g_jwt_issuer->getValue());
        verifier.verify(dec);
        if (out_uid) {
            if (dec.has_payload_claim("uid")) {
                *out_uid = dec.get_payload_claim("uid").as_string();
            } else {
                *out_uid = "";
            }
        }
        return true;
    } catch (const std::exception& e) {
        CIM_LOG_WARN(g_logger) << "jwt verify failed: " << e.what();
        return false;
    }
}

}  // namespace CIM
