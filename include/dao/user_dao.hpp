#pragma once

#include <cstdint>
#include <ctime>
#include <optional>
#include <string>

namespace CIM::dao
{

struct User
{
    uint64_t id = 0;             // 用户ID
    std::string mobile;          // 手机号
    std::string email;           // 邮箱
    std::string nickname;        // 昵称
    std::string password_hash;   // 密码哈希
    std::string password_salt;   // 密码盐（兼容旧表结构，实际哈希可能已包含盐）
    std::string avatar;          // 头像
    int32_t gender = 0;          // 性别
    std::string motto;           // 个性签名
    int32_t status = 1;          // 状态
    std::time_t created_at = 0;  // 创建时间
    std::time_t updated_at = 0;  // 更新时间
};

class UserDAO
{
   public:
    // 创建新用户，成功时返回true并设置out_id
    static bool Create(const User& u, uint64_t& out_id,
                       std::string* err = nullptr);

    // 根据手机号获取用户信息
    static bool GetByMobile(const std::string& mobile, User& out);

    // 根据用户ID获取用户信息
    static bool GetById(uint64_t id, User& out);

    // 更新用户密码
    static bool UpdatePassword(uint64_t id,
                               const std::string& new_password_hash,
                               std::string* err = nullptr);
};

}  // namespace CIM::dao
