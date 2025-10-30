#include "app/auth_service.hpp"

#include "macro.hpp"
#include "util/password.hpp"
#include "util/hash_util.hpp"
#include "crypto_module.hpp"

namespace CIM::app
{
    static auto g_logger = CIM_LOG_NAME("auth");

    AuthResult AuthService::Register(const std::string &mobile,
                                     const std::string &password,
                                     const std::string &email,
                                     const std::string &nickname)
    {
        AuthResult r;
        /*参数校验*/
        if (mobile.empty() || password.empty() || email.empty() || nickname.empty())
        {
            r.err = "invalid mobile or password";
            return r;
        }

        /*检查手机号是否已注册*/
        CIM::dao::User exist;
        if (CIM::dao::UserDAO::GetByMobile(mobile, exist))
        {
            r.err = "mobile already registered";
            return r;
        }

        /*使用Base64解码密码并RSA解密*/
        // Base64 解码
        std::string cipher_bin = CIM::base64decode(password);
        if (cipher_bin.empty())
        {
            r.err = "invalid password format";
            return r;
        }
        // 私钥解密
        auto cm = CIM::CryptoModule::Get();
        if (!cm || !cm->isReady())
        {
            r.err = "crypto module not ready";
            return r;
        }
        std::string decrypted_pwd = "";
        if (!cm->PrivateDecrypt(cipher_bin, decrypted_pwd))
        {
            r.err = "password decrypt failed";
            return r;
        }

        /*生成密码哈希*/
        auto ph = CIM::util::Password::Hash(decrypted_pwd);
        if (ph.empty())
        {
            r.err = "hash failed";
            return r;
        }

        /*创建用户*/
        CIM::dao::User u;
        u.mobile = mobile;
        u.email = email;
        u.password_hash = ph;
        u.nickname = nickname;
        u.gender = 0;
        u.status = 1;

        uint64_t new_id = 0;
        std::string err;
        if (!CIM::dao::UserDAO::Create(u, new_id, &err))
        {
            r.err = err.empty() ? std::string("create user failed") : err;
            return r;
        }
        u.id = new_id;
        r.ok = true;
        r.user = std::move(u);
        return r;
    }

    AuthResult AuthService::Authenticate(const std::string &mobile,
                                         const std::string &password)
    {
        AuthResult r;
        /*密码解密*/
        // Base64 解码
        std::string cipher_bin = CIM::base64decode(password);
        if (cipher_bin.empty())
        {
            r.err = "invalid password format";
            return r;
        }

        // 私钥解密
        auto cm = CIM::CryptoModule::Get();
        if (!cm || !cm->isReady())
        {
            r.err = "crypto module not ready";
            return r;
        }
        std::string decrypted_pwd = "";
        if (!cm->PrivateDecrypt(cipher_bin, decrypted_pwd))
        {
            r.err = "password decrypt failed";
            return r;
        }

        /*获取用户信息*/
        CIM::dao::User u;
        if (!CIM::dao::UserDAO::GetByMobile(mobile, u))
        {
            r.err = "user not found";
            return r;
        }

        /*验证密码*/
        if (!CIM::util::Password::Verify(decrypted_pwd, u.password_hash))
        {
            r.err = "invalid password";
            return r;
        }

        /*检查用户状态*/
        if (u.status != 1)
        {
            r.err = "user disabled";
            return r;
        }
        r.ok = true;
        r.user = std::move(u);
        return r;
    }

    AuthResult AuthService::ChangePassword(uint64_t uid,
                                           const std::string &old_password,
                                           const std::string &new_password)
    {
        AuthResult r;
        if (new_password.size() < 6)
        {
            r.err = "new_password too short";
            return r;
        }
        CIM::dao::User u;
        if (!CIM::dao::UserDAO::GetById(uid, u))
        {
            r.err = "user not found";
            return r;
        }
        if (!CIM::util::Password::Verify(old_password, u.password_hash))
        {
            r.err = "old_password mismatch";
            return r;
        }
        auto ph = CIM::util::Password::Hash(new_password);
        if (ph.empty())
        {
            r.err = "hash failed";
            return r;
        }
        std::string err;
        if (!CIM::dao::UserDAO::UpdatePassword(uid, ph, &err))
        {
            r.err = err.empty() ? std::string("update failed") : err;
            return r;
        }
        r.ok = true;
        r.user = std::move(u);
        return r;
    }

} // namespace CIM::app
