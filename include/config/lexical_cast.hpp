#pragma once

#include "logger_manager.hpp"
#include <boost/lexical_cast.hpp>
#include <list>
#include <map>
#include <set>
#include <unordered_set>

namespace sylar
{
    /**
     * 不同类型的数据转换
     * 主要思路：
     *     使用模板的全特化和偏特化满足不同类型的转换
     * 基本类型：使用 boost::lexical_cast 实现
     * 容器类型/自定义类型：使用 YAML 进行序列化和反序列化
     */

    // 主模板，实现通用数据转换 F from_type  T to_type
    template <class F, class T>
    class LexicalCast
    {
    public:
        T operator()(const F &from) const
        {
            return boost::lexical_cast<T>(from);
        }
    };

    // 模板偏特化，实现字符串转化为vector
    template <class T>
    class LexicalCast<std::string, std::vector<T>>
    {
    public:
        std::vector<T> operator()(const std::string &v) const
        {
            // 将字符串解析为YAML节点， node 相当于一个数组
            YAML::Node node = YAML::Load(v);
            typename std::vector<T> vec;
            std::stringstream ss;
            for (size_t i = 0; i < node.size(); ++i)
            {
                ss.str("");    // 清空字符串流
                ss << node[i]; // 将节点元素输出到字符串流中（将YAML节点转化为字符串形式）
                // 将字符串转为 T 类型后添加到vector中
                vec.push_back(LexicalCast<std::string, T>()(ss.str()));
            }
            return vec;
        }
    };

    // 模板偏特化，实现vector转化为字符串
    template <class T>
    class LexicalCast<std::vector<T>, std::string>
    {
    public:
        std::string operator()(const std::vector<T> &v) const
        {
            YAML::Node node;
            for (auto &i : v)
            {
                node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    // 模板偏特化，实现字符串转化为 list
    template <class T>
    class LexicalCast<std::string, std::list<T>>
    {
    public:
        std::list<T> operator()(const std::string &v) const
        {
            // 将字符串解析为YAML节点， node 相当于一个数组
            YAML::Node node = YAML::Load(v);
            typename std::list<T> list;
            std::stringstream ss;
            for (size_t i = 0; i < node.size(); ++i)
            {
                ss.str("");    // 清空字符串流
                ss << node[i]; // 将节点元素输出到字符串流中（将YAML节点转化为字符串形式）
                // 将字符串转为 T 类型后添加到 list 中
                list.push_back(LexicalCast<std::string, T>()(ss.str()));
            }
            return list;
        }
    };

    // 模板偏特化，实现 list 转化为字符串
    template <class T>
    class LexicalCast<std::list<T>, std::string>
    {
    public:
        std::string operator()(const std::list<T> &v) const
        {
            YAML::Node node;
            for (auto &i : v)
            {
                node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    // 模板偏特化，实现字符串转化为 set
    template <class T>
    class LexicalCast<std::string, std::set<T>>
    {
    public:
        std::set<T> operator()(const std::string &v) const
        {
            // 将字符串解析为YAML节点， node 相当于一个数组
            YAML::Node node = YAML::Load(v);
            typename std::set<T> set;
            std::stringstream ss;
            for (size_t i = 0; i < node.size(); ++i)
            {
                ss.str("");    // 清空字符串流
                ss << node[i]; // 将节点元素输出到字符串流中（将YAML节点转化为字符串形式）
                // 将字符串转为 T 类型后添加到 set 中
                set.insert(LexicalCast<std::string, T>()(ss.str()));
            }
            return set;
        }
    };

    // 模板偏特化，实现 set 转化为字符串
    template <class T>
    class LexicalCast<std::set<T>, std::string>
    {
    public:
        std::string operator()(const std::set<T> &v) const
        {
            YAML::Node node;
            for (auto &i : v)
            {
                node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    // 模板偏特化，实现字符串转化为 unordered_set
    template <class T>
    class LexicalCast<std::string, std::unordered_set<T>>
    {
    public:
        std::unordered_set<T> operator()(const std::string &v) const
        {
            // 将字符串解析为YAML节点， node 相当于一个数组
            YAML::Node node = YAML::Load(v);
            typename std::unordered_set<T> unordered_set;
            std::stringstream ss;
            for (size_t i = 0; i < node.size(); ++i)
            {
                ss.str("");    // 清空字符串流
                ss << node[i]; // 将节点元素输出到字符串流中（将YAML节点转化为字符串形式）
                // 将字符串转为 T 类型后添加到 unordered_set 中
                unordered_set.insert(LexicalCast<std::string, T>()(ss.str()));
            }
            return unordered_set;
        }
    };

    // 模板偏特化，实现 unordered_set 转化为字符串
    template <class T>
    class LexicalCast<std::unordered_set<T>, std::string>
    {
    public:
        std::string operator()(const std::unordered_set<T> &v) const
        {
            YAML::Node node;
            for (auto &i : v)
            {
                node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    // 模板偏特化，实现字符串转化为 map
    template <class T>
    class LexicalCast<std::string, std::map<std::string, T>>
    {
    public:
        std::map<std::string, T> operator()(const std::string &v) const
        {
            // 将字符串解析为YAML节点， node 相当于一个数组
            YAML::Node node = YAML::Load(v);
            typename std::map<std::string, T> map;
            std::stringstream ss;
            for (auto it : node)
            {
                ss.str("");      // 清空字符串流
                ss << it.second; // 将节点元素输出到字符串流中（将YAML节点转化为字符串形式）
                // 将字符串转为 T 类型后添加到 map 中
                map.insert(std::make_pair(it.first.Scalar(), LexicalCast<std::string, T>()(ss.str())));
            }
            return map;
        }
    };

    // 模板偏特化，实现 map 转化为字符串
    template <class T>
    class LexicalCast<std::map<std::string, T>, std::string>
    {
    public:
        std::string operator()(const std::map<std::string, T> &v) const
        {
            YAML::Node node;
            for (auto &i : v)
            {
                node[i.first] = YAML::Load(LexicalCast<T, std::string>()(i.second));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    // 模板偏特化，实现字符串转化为 map
    template <class T>
    class LexicalCast<std::string, std::unordered_map<std::string, T>>
    {
    public:
        std::unordered_map<std::string, T> operator()(const std::string &v) const
        {
            // 将字符串解析为YAML节点， node 相当于一个数组
            YAML::Node node = YAML::Load(v);
            typename std::unordered_map<std::string, T> unordered_map;
            std::stringstream ss;
            for (auto it : node)
            {
                ss.str("");      // 清空字符串流
                ss << it.second; // 将节点元素输出到字符串流中（将YAML节点转化为字符串形式）
                // 将字符串转为 T 类型后添加到 unordered_map 中
                unordered_map.insert(std::make_pair(it.first.Scalar(), LexicalCast<std::string, T>()(ss.str())));
            }
            return unordered_map;
        }
    };

    // 模板偏特化，实现 unordered_map 转化为字符串
    template <class T>
    class LexicalCast<std::unordered_map<std::string, T>, std::string>
    {
    public:
        std::string operator()(const std::unordered_map<std::string, T> &v) const
        {
            YAML::Node node;
            for (auto &i : v)
            {
                node[i.first] = YAML::Load(LexicalCast<T, std::string>()(i.second));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    // 为LogAppenderDefine实现YAML序列化支持
    template <>
    class LexicalCast<LogAppenderDefine, std::string>
    {
    public:
        std::string operator()(const LogAppenderDefine &val) const
        {
            YAML::Node node;
            if (val.type == 1)
            {
                node["type"] = "FileLogAppender";
            }
            else if (val.type == 2)
            {
                node["type"] = "StdoutLogAppender";
            }
            node["level"] = LogLevel::ToString(val.level);
            node["formatter"] = val.formatter;
            node["path"] = val.path;
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    template <>
    class LexicalCast<std::string, LogAppenderDefine>
    {
    public:
        LogAppenderDefine operator()(const std::string &val) const
        {
            YAML::Node node = YAML::Load(val);
            LogAppenderDefine lad;
            // 使用IsDefined()的原因是：防止元素不存在而创建该元素
            if (node["type"].IsDefined())
            {
                if (node["type"].as<std::string>() == "FileLogAppender")
                {
                    lad.type = 1;
                }
                else if (node["type"].as<std::string>() == "StdoutLogAppender")
                {
                    lad.type = 2;
                }
            }
            if (node["level"].IsDefined())
            {
                lad.level = LogLevel::FromString(node["level"].as<std::string>());
            }
            if (node["formatter"].IsDefined())
            {
                lad.formatter = node["formatter"].as<std::string>();
            }
            if (node["path"].IsDefined())
            {
                lad.path = node["path"].as<std::string>();
            }
            return lad;
        }
    };

    template <>
    class LexicalCast<LogDefine, std::string>
    {
    public:
        std::string operator()(const LogDefine &val) const
        {
            YAML::Node node;
            node["name"] = val.name;
            node["level"] = LogLevel::ToString(val.level);
            node["formatter"] = val.formatter;
            for (const auto &appender : val.appenders)
            {
                YAML::Node appender_node;
                if (appender.type == 1)
                {
                    appender_node["type"] = "FileLogAppender";
                }
                else if (appender.type == 2)
                {
                    appender_node["type"] = "StdoutLogAppender";
                }
                appender_node["level"] = LogLevel::ToString(appender.level);
                appender_node["formatter"] = appender.formatter;
                appender_node["path"] = appender.path;
                node["appenders"].push_back(appender_node);
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    template <>
    class LexicalCast<std::string, LogDefine>
    {
    public:
        LogDefine operator()(const std::string &val) const
        {
            YAML::Node node = YAML::Load(val);
            LogDefine ld;
            if (node["name"].IsDefined())
            {
                ld.name = node["name"].as<std::string>();
            }
            if (node["level"].IsDefined())
            {
                ld.level = LogLevel::FromString(node["level"].as<std::string>());
            }
            if (node["formatter"].IsDefined())
            {
                ld.formatter = node["formatter"].as<std::string>();
            }
            if (node["appenders"].IsDefined())
            {
                for (const auto &appender_node : node["appenders"])
                {
                    LogAppenderDefine lad;
                    if (appender_node["type"].IsDefined())
                    {
                        if (appender_node["type"].as<std::string>() == "FileLogAppender")
                        {
                            lad.type = 1;
                        }
                        else if (appender_node["type"].as<std::string>() == "StdoutLogAppender")
                        {
                            lad.type = 2;
                        }
                    }
                    if (appender_node["level"].IsDefined())
                    {
                        lad.level = LogLevel::FromString(appender_node["level"].as<std::string>());
                    }
                    if (appender_node["formatter"].IsDefined())
                    {
                        lad.formatter = appender_node["formatter"].as<std::string>();
                    }
                    if (appender_node["path"].IsDefined())
                    {
                        lad.path = appender_node["path"].as<std::string>();
                    }
                    ld.appenders.push_back(lad);
                }
            }
            return ld;
        }
    };
}