#include "dao/user_dao.hpp"

#include "db/mysql.hpp"
#include "macro.hpp"

namespace CIM::dao
{

static auto g_logger = CIM_LOG_NAME("db");

static const char* kDBName = "default";

bool UserDAO::Create(const User& u, uint64_t& out_id, std::string* err)
{
    auto db = CIM::MySQLMgr::GetInstance()->get(kDBName);
    if (!db)
    {
        if (err) *err = "no mysql connection";
        return false;
    }

    const char* sql =
        "INSERT INTO users (mobile, email, password_hash, nickname, avatar, gender, motto, status) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?)";
    auto stmt = db->prepare(sql);
    if (!stmt)
    {
        if (err) *err = "prepare failed";
        return false;
    }
    stmt->bindString(1, u.mobile);
    if (!u.email.empty())
        stmt->bindString(2, u.email);
    else
        stmt->bindNull(2);
    stmt->bindString(3, u.password_hash);
    stmt->bindString(4, u.nickname);
    stmt->bindString(5, u.avatar);
    stmt->bindInt32(6, u.gender);
    stmt->bindString(7, u.motto);
    stmt->bindInt32(8, u.status);

    if (stmt->execute() != 0)
    {
        if (err) *err = stmt->getErrStr();
        return false;
    }
    out_id = static_cast<uint64_t>(stmt->getLastInsertId());
    return true;
}

bool UserDAO::GetByMobile(const std::string& mobile, User& out)
{
    auto db = CIM::MySQLMgr::GetInstance()->get(kDBName);
    if (!db) return false;
    const char* sql =
        "SELECT id, mobile, email, password_hash, nickname, avatar, gender, motto, status, created_at, updated_at FROM "
        "users WHERE mobile = ? LIMIT 1";
    auto stmt = db->prepare(sql);
    if (!stmt) return false;
    stmt->bindString(1, mobile);
    auto res = stmt->query();
    if (!res) return false;
    if (!res->next()) return false;
    out.id = static_cast<uint64_t>(res->getUint64(0));
    out.mobile = res->getString(1);
    out.email = res->isNull(2) ? std::string() : res->getString(2);
    out.password_hash = res->getString(3);
    out.nickname = res->getString(4);
    out.avatar = res->getString(5);
    out.gender = static_cast<int32_t>(res->getInt32(6));
    out.motto = res->getString(7);
    out.status = static_cast<int32_t>(res->getInt32(8));
    out.created_at = res->getTime(9);
    out.updated_at = res->getTime(10);
    return true;
}

bool UserDAO::GetById(uint64_t id, User& out)
{
    auto db = CIM::MySQLMgr::GetInstance()->get(kDBName);
    if (!db) return false;
    const char* sql =
        "SELECT id, mobile, email, password_hash, nickname, avatar, gender, motto, status, created_at, updated_at FROM "
        "users WHERE id = ? LIMIT 1";
    auto stmt = db->prepare(sql);
    if (!stmt) return false;
    stmt->bindUint64(1, id);
    auto res = stmt->query();
    if (!res) return false;
    if (!res->next()) return false;
    out.id = static_cast<uint64_t>(res->getUint64(0));
    out.mobile = res->getString(1);
    out.email = res->isNull(2) ? std::string() : res->getString(2);
    out.password_hash = res->getString(3);
    out.nickname = res->getString(4);
    out.avatar = res->getString(5);
    out.gender = static_cast<int32_t>(res->getInt32(6));
    out.motto = res->getString(7);
    out.status = static_cast<int32_t>(res->getInt32(8));
    out.created_at = res->getTime(9);
    out.updated_at = res->getTime(10);
    return true;
}

bool UserDAO::UpdatePassword(uint64_t id, const std::string& new_password_hash, std::string* err)
{
    auto db = CIM::MySQLMgr::GetInstance()->get(kDBName);
    if (!db)
    {
        if (err) *err = "no mysql connection";
        return false;
    }
    const char* sql = "UPDATE users SET password_hash = ?, updated_at = NOW() WHERE id = ?";
    auto stmt = db->prepare(sql);
    if (!stmt)
    {
        if (err) *err = "prepare failed";
        return false;
    }
    stmt->bindString(1, new_password_hash);
    stmt->bindUint64(2, id);
    if (stmt->execute() != 0)
    {
        if (err) *err = stmt->getErrStr();
        return false;
    }
    return true;
}

}  // namespace CIM::dao
