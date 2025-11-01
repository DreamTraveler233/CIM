#pragma once

#include <cstdint>
#include <ctime>
#include <optional>
#include <string>

namespace CIM::dao {

struct User {
    uint64_t id = 0;                // 用户ID，自增主键
    std::string mobile;             // 手机号，唯一，用于登录
    std::string email;              // 邮箱，可选，唯一，用于找回密码
    std::string nickname;           // 昵称，显示名称
    std::string password_hash;      // 密码哈希，使用PBKDF2+盐（盐嵌入其中）
    std::string avatar;             // 头像URL，可选
    std::string motto;              // 个性签名，可选
    int32_t gender = 0;             // 性别：0未知 1男 2女
    int32_t is_robot = 0;           // 是否机器人：0否 1是
    int32_t is_qiye = 0;            // 是否企业用户：0否 1是
    int32_t status = 1;             // 账户状态：1正常 2禁用
    std::time_t last_login_at = 0;  // 最后登录时间
    std::time_t created_at = 0;     // 创建时间
    std::time_t updated_at = 0;     // 更新时间
};

class UserDAO {
   public:
    // 创建新用户，成功时返回true并设置out_id
    static bool Create(const User& u, uint64_t& out_id, std::string* err = nullptr);

    // 根据手机号获取用户信息
    static bool GetByMobile(const std::string& mobile, User& out);

    // 根据用户ID获取用户信息
    static bool GetById(uint64_t id, User& out);

    // 更新用户密码
    static bool UpdatePassword(uint64_t id, const std::string& new_password_hash,
                               std::string* err = nullptr);

    // 更新最后登录时间
    static bool UpdateLastLoginAt(uint64_t id, std::time_t last_login_at,
                                  std::string* err = nullptr);

    // 更新用户状态
    static bool UpdateStatus(uint64_t id, int32_t status, std::string* err = nullptr);

    // 更新用户信息（昵称、头像、签名等）
    static bool UpdateProfile(uint64_t id, const std::string& nickname, const std::string& avatar,
                              const std::string& motto, std::string* err = nullptr);
};

}  // namespace CIM::dao
