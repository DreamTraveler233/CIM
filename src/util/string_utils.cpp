#include "string_util.hpp"

namespace sylar
{
    bool StringUtil::StartsWith(const std::string &str, const std::string &sub)
    {
        // 如果字串为空，直接返回 true
        if (sub.empty())
        {
            return true;
        }
        // 如果字符串为空或字符串比子串短，直接返回 false
        auto strLen = str.size();
        auto subLen = sub.size();
        if (str.empty() || strLen < subLen)
        {
            return false;
        }
        // 用 str 从下标 0 开始、长度为 subLen 的子串，与 sub 进行比较
        return str.compare(0, subLen, sub) == 0;
    }

    bool StringUtil::EndsWith(const std::string &str, const std::string &sub)
    {
        // 如果字串为空，直接返回 true
        if (sub.empty())
        {
            return true;
        }
        // 如果字符串为空或字符串比子串短，直接返回 false
        auto strLen = str.size();
        auto subLen = sub.size();
        if (str.empty() || strLen < subLen)
        {
            return false;
        }
        // 用 str 从下标 0 开始、长度为 subLen 的子串，与 sub 进行比较
        return str.compare(strLen - subLen, subLen, sub) == 0;
    }

    std::string StringUtil::FilePath(const std::string &path)
    {
        auto pos = path.find_last_of("/\\"); // 查找最后一个'/'或'\'的位置（兼容Linux和Windows路径）
        if (pos != std::string::npos)        // 如果找到了分隔符
        {
            return path.substr(0, pos + 1); // 返回从开头到分隔符前的所有字符（即目录部分）
        }
        else // 如果没有找到分隔符
        {
            return "./"; // 返回当前目录
        }
    }

    std::string StringUtil::FileNameExt(const std::string &path)
    {
        auto pos = path.find_last_of("/\\"); // 查找路径中最后一个'/'或'\'的位置（兼容Linux和Windows路径）
        if (pos != std::string::npos)        // 如果找到了分隔符
        {
            if (pos + 1 < path.size()) // 并且分隔符后还有内容
            {
                return path.substr(pos + 1); // 返回分隔符后的所有内容（即文件名+扩展名）
            }
        }
        return path; // 如果没有分隔符，直接返回原字符串（说明本身就是文件名）
    }

    std::string StringUtil::FileName(const std::string &path)
    {
        std::string file_name = FileNameExt(path); // 先获取文件名（含扩展名）
        auto pos = file_name.find_last_of(".");    // 查找最后一个'.'的位置（即扩展名前的点）
        if (pos != std::string::npos)              // 如果找到了'.'
        {
            if (pos != 0) // 并且'.'不是第一个字符（防止隐藏文件如 .bashrc）
            {
                return file_name.substr(0, pos); // 返回从头到'.'前的内容（即文件名，不含扩展名）
            }
        }
        return file_name; // 如果没有'.'，或'.'在第一个字符，直接返回整个文件名
    }

    std::string StringUtil::Extension(const std::string &path)
    {
        std::string file_name = FileNameExt(path); // 先获取文件名（含扩展名）
        auto pos = file_name.find_last_of(".");    // 查找最后一个'.'的位置
        if (pos != std::string::npos)              // 如果找到了'.'
        {
            if (pos != 0 && pos + 1 < path.size()) // '.'不是第一个字符，且后面还有内容
            {
                return file_name.substr(pos); // 返回从'.'开始到结尾的内容（即扩展名，包含点）
            }
        }
        return std::string(); // 没有扩展名则返回空字符串
    }

    std::vector<std::string> StringUtil::SplitString(const std::string &str, const std::string &delimiter)
    {
        std::vector<std::string> result; // 存放分割后的子串
        // 如果分隔符为空，直接返回空vector
        if (delimiter.empty())
        {
            return result;
        }
        size_t last = 0; // 上一次分割结束的位置
        size_t next = 0; // 下一个分隔符出现的位置
        // 循环查找分隔符并分割
        while ((next = str.find(delimiter, last)) != std::string::npos)
        {
            if (next > last) // 分隔符前有内容
            {
                // 截取从last到next之间的子串，加入结果
                result.emplace_back(str.substr(last, next - last));
            }
            // 更新last到下一个分隔符后面
            last = next + delimiter.size();
        }
        // 处理最后一段（如果last还没到末尾）
        if (last < str.size())
        {
            result.emplace_back(str.substr(last));
        }
        return result; // 返回所有分割结果
    }
}