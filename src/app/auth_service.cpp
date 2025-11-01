#include "app/auth_service.hpp"

#include "crypto_module.hpp"
#include "macro.hpp"
#include "util/hash_util.hpp"
#include "util/password.hpp"

namespace CIM::app {
static auto g_logger = CIM_LOG_NAME("root");

AuthResult AuthService::Register(const std::string& mobile, const std::string& password,
                                 const std::string& email, const std::string& nickname) {
    AuthResult result;

    /*检查手机号是否已注册*/
    CIM::dao::User exist;
    if (CIM::dao::UserDAO::GetByMobile(mobile, exist)) {
        result.err = "手机号已被注册！";
        return result;
    }

    /*使用Base64解码密码并RSA解密*/
    // Base64 解码
    std::string cipher_bin = CIM::base64decode(password);
    if (cipher_bin.empty()) {
        result.err = "密码解码失败！";
        return result;
    }
    // 私钥解密
    auto cm = CIM::CryptoModule::Get();
    if (!cm || !cm->isReady()) {
        result.err = "密钥模块未加载！";
        return result;
    }
    std::string decrypted_pwd = "";
    if (!cm->PrivateDecrypt(cipher_bin, decrypted_pwd)) {
        result.err = "密码解密失败！";
        return result;
    }

    /*生成密码哈希*/
    auto ph = CIM::util::Password::Hash(decrypted_pwd);
    if (ph.empty()) {
        result.err = "密码哈希生成失败！";
        return result;
    }

    /*创建用户*/
    CIM::dao::User u;
    u.mobile = mobile;
    u.email = email;
    u.password_hash = ph;
    u.nickname = nickname;

    uint64_t new_id = 0;
    std::string err;
    if (!CIM::dao::UserDAO::Create(u, new_id, &err)) {
        result.err = err.empty() ? std::string("创建用户失败！") : err;
        return result;
    }
    u.id = new_id;
    result.ok = true;
    result.user = std::move(u);
    return result;
}

AuthResult AuthService::Authenticate(const std::string& mobile, const std::string& password) {
    AuthResult result;

    /*密码解密*/
    // Base64 解码
    std::string cipher_bin = CIM::base64decode(password);
    if (cipher_bin.empty()) {
        result.err = "密码解码失败！";
        return result;
    }

    // 私钥解密
    auto cm = CIM::CryptoModule::Get();
    if (!cm || !cm->isReady()) {
        result.err = "密钥模块未加载！";
        return result;
    }
    std::string decrypted_pwd = "";
    if (!cm->PrivateDecrypt(cipher_bin, decrypted_pwd)) {
        result.err = "密码解密失败！";
        return result;
    }

    /*获取用户信息*/
    CIM::dao::User u;
    if (!CIM::dao::UserDAO::GetByMobile(mobile, u)) {
        result.err = "账号或密码错误！";
        return result;
    }

    /*验证密码*/
    if (!CIM::util::Password::Verify(decrypted_pwd, u.password_hash)) {
        result.err = "账号或密码错误！";
        return result;
    }

    /*检查用户状态*/
    if (u.status != 1) {
        result.err = "账户被禁用!";
        return result;
    }
    result.ok = true;
    result.user = std::move(u);
    return result;
}

AuthResult AuthService::Forget(const std::string& mobile, const std::string& new_password) {
    AuthResult result;

    /*密码解密*/
    // Base64 解码
    std::string cipher_bin = CIM::base64decode(new_password);
    if (cipher_bin.empty()) {
        result.err = "密码解码失败！";
        return result;
    }

    // 私钥解密
    auto cm = CIM::CryptoModule::Get();
    if (!cm || !cm->isReady()) {
        result.err = "密钥模块未加载！";
        return result;
    }
    std::string decrypted_pwd = "";
    if (!cm->PrivateDecrypt(cipher_bin, decrypted_pwd)) {
        result.err = "密码解密失败！";
        return result;
    }

    /*检查手机号是否存在*/
    CIM::dao::User u;
    if (!CIM::dao::UserDAO::GetByMobile(mobile, u)) {
        result.err = "手机号不存在！";
        return result;
    }

    /*生成新密码哈希*/
    auto ph = CIM::util::Password::Hash(decrypted_pwd);
    if (ph.empty()) {
        result.err = "密码哈希生成失败！";
        return result;
    }

    /*更新用户密码*/
    std::string err;
    if (!CIM::dao::UserDAO::UpdatePassword(u.id, ph, &err)) {
        result.err = err.empty() ? std::string("更新密码失败！") : err;
        return result;
    }

    result.ok = true;
    result.user = std::move(u);
    return result;
}

AuthResult AuthService::ChangePassword(uint64_t uid, const std::string& old_password,
                                       const std::string& new_password) {
    AuthResult result;
    if (new_password.size() < 6) {
        result.err = "new_password too short";
        return result;
    }
    CIM::dao::User u;
    if (!CIM::dao::UserDAO::GetById(uid, u)) {
        result.err = "user not found";
        return result;
    }
    if (!CIM::util::Password::Verify(old_password, u.password_hash)) {
        result.err = "old_password mismatch";
        return result;
    }
    auto ph = CIM::util::Password::Hash(new_password);
    if (ph.empty()) {
        result.err = "hash failed";
        return result;
    }
    std::string err;
    if (!CIM::dao::UserDAO::UpdatePassword(uid, ph, &err)) {
        result.err = err.empty() ? std::string("update failed") : err;
        return result;
    }
    result.ok = true;
    result.user = std::move(u);
    return result;
}

}  // namespace CIM::app
