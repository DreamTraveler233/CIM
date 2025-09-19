#pragma once

#include "yaml-cpp/yaml.h"
#include "config_var_base.hpp"
#include "config_var.hpp"
#include "log.hpp"

namespace sylar
{
    // 配置管理器
    class Config
    {
    public:
        using ConfigVarMap = std::map<std::string, ConfigVarBase::ptr>;

        /**
         * @brief 查找或创建配置项
         * @tparam T 配置项的数据类型
         * @param name 配置项名称
         * @param default_value 配置项默认值
         * @param description 配置项描述信息
         * @return 返回配置项的智能指针
         *
         * 1、根据名称查找已存在的配置项，如果找到且类型匹配则返回
         * 2、如果找到但类型不匹配则记录错误日志
         * 3、如果未找到且名称合法，则创建新的配置项并返回
         * 4、如果名称不合法则抛出异常
         *
         */
        template <class T>
        static typename ConfigVar<T>::ptr Lookup(const std::string &name,
                                                 const T &default_value,
                                                 const std::string &description = "")
        {
            // 根据配置项名称在存储容器中查找
            auto it = GetDatas().find(name);
            // 如果找到了同名配置项
            if (it != GetDatas().end())
            {
                // 尝试将找到的配置项基类指针转换为指定类型T的配置项指针
                // dynamic_pointer_cast会在运行时检查类型兼容性，如果类型不匹配会返回nullptr
                auto tmp = std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
                // 如果类型转换成功，说明找到了同名且同类型的配置项
                if (tmp)
                {
                    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "Lookup name = " << name << " exists";
                    return tmp;
                }
                // 如果类型转换失败，说明虽然找到了同名配置项，但其实际类型与请求的类型T不匹配
                else
                {
                    SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "Lookup name = " << name << " exists but type not "
                                                      << typeid(T).name() << " real_type = " << it->second->getTypeName()
                                                      << " " << "value = " << it->second->toString();
                    // 抛出异常
                    throw std::runtime_error("Config variable '" + name + "' exists but type mismatch. Requested: " + std::string(typeid(T).name()) + ", Actual: " + it->second->getTypeName());
                }
            }

            // 如果没有到了同名配置项，则创建该配置项
            // 检查配置项名称是否合法，只能包含字母、数字、下划线和点号
            if (name.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789._") != std::string::npos)
            {
                SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "lookup name invalid name=" << name;
                throw std::invalid_argument(name); // 抛出异常
            }

            // 创建新的配置项
            typename ConfigVar<T>::ptr v(new ConfigVar<T>(name, default_value, description));
            GetDatas()[name] = v;
            return v;
        }

        /**
         * @brief 根据名称查找配置项
         * @tparam T 配置项的数据类型
         * @param name 配置项名称
         * @return 返回找到的配置项指针，如果未找到则返回nullptr
         */
        template <class T>
        static typename ConfigVar<T>::ptr Lookup(const std::string &name)
        {
            auto it = GetDatas().find(name);
            if (it == GetDatas().end())
            {
                return nullptr;
            }
            // 尝试将配置项转换为指定数据类型
            return std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
        }

        /**
         * @brief 根据名称查找配置项（基类版本）
         * @param name 配置项名称
         * @return 找到的配置项指针，如果未找到则返回nullptr
         */
        static ConfigVarBase::ptr LookupBase(const std::string &name);

        /**
         * @brief 从YAML配置文件中加载配置项
         * @param root YAML配置文件的根节点
         */
        static void LoadFromYaml(const YAML::Node &root);

    private:
        /**
         * 通过静态函数和静态局部变量来解决静态初始化顺序问题，函数首次调用时会初始化这个静态变量，
         * 之后的调用都会返回同一个实例的引用，这样防止了当该静态变量没有初始化就被使用
         */
        static ConfigVarMap &GetDatas()
        {
            static ConfigVarMap m_datas; // 配置项存储
            return m_datas;
        }
    };
}