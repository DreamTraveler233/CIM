#pragma once

#include "lexical_cast.hpp"
#include "log.hpp"

namespace sylar
{
    //
    template <class T,
              class fromStr = LexicalCast<std::string, T>,
              class toStr = LexicalCast<T, std::string>>
    class ConfigVar : public ConfigVarBase
    {
    public:
        using ptr = std::shared_ptr<ConfigVar>;
        using ConfigChangeCb = std::function<void(const T &old_value, const T &new_value)>;

        ConfigVar(const std::string &name, const T &default_value, const std::string &description)
            : ConfigVarBase(name, description),
              m_val(default_value)
        {
        }

        /**
         * @brief 将配置项的值转换为字符串表示
         * @return std::string 配置值的字符串表示，转换失败时返回空字符串
         *
         * 1、使用toStr()获取转换函数，将成员变量m_val转为字符串
         * 2、若转换过程出现异常，则记录错误日志并返回空字符串
         * 3、主要用于配置项的序列化输出
         */
        std::string toString() override
        {
            try
            {
                return toStr()(m_val);
            }
            catch (std::exception &e)
            {
                SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ConfigVar::toString exception"
                                                  << e.what() << " convert: "
                                                  << typeid(m_val).name()
                                                  << " to string";
            }
            return "";
        }

        /**
         * @brief 将字符串转换为配置项的值
         * @param[in] val 待转换的字符串
         * @return 转换成功返回true，失败返回false
         *
         * 1、使用fromStr()将输入字符串转换为目标类型值
         * 2、调用setValue()设置转换后的值
         * 3、捕获转换异常并记录错误日志
         * 4、成功转换返回true，失败返回false
         */
        bool fromString(const std::string &val) override
        {
            try
            {
                setValue(fromStr()(val));
                return true;
            }
            catch (std::exception &e)
            {
                SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ConfigVar::fromString exception"
                                                  << e.what() << " convert: string to"
                                                  << typeid(m_val).name() << " - " << val;
            }
            return false;
        }

        std::string getTypeName() const override
        {
            return typeid(T).name();
        }

        void setValue(const T &v)
        {
            if (v == m_val)
            {
                return;
            }
            // 配置发生改变
            // SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "Changing config variable: " << getName()
            //                                  << " from: " << toStr()(m_val)
            //                                  << " to: " << toStr()(v);
            // SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "Changing config :" << loggerMgr::getInstance()->toYamlString();
            for (auto &i : m_cbs)
            {
                i.second(m_val, v);
            }
            m_val = v;
        }
        T getValue() const { return m_val; }

        void addListener(uint64_t key, const ConfigChangeCb &cb)
        {
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "Adding listener for config variable: "
                                             << getName() << " with key: " << key;
            m_cbs[key] = cb;
        }
        void delListener(uint64_t key)
        {
            if (m_cbs.find(key) != m_cbs.end())
            {
                SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "Removing listener for config variable: "
                                                 << getName() << " with key: " << key;
                m_cbs.erase(key);
            }
            else
            {
                SYLAR_LOG_WARN(SYLAR_LOG_ROOT()) << "Trying to remove non-existent listener for config variable: "
                                                 << getName() << " with key: " << key;
            }
        }
        void clearListener()
        {
            m_cbs.clear();
        }
        ConfigChangeCb getListener(uint64_t key)
        {
            auto it = m_cbs.find(key);
            return it == m_cbs.end() ? nullptr : it->second;
        }

    private:
        T m_val; // 配置值
        // 变更回调函数组，uint64_t key，要求唯一，一般可用hash
        std::map<uint64_t, ConfigChangeCb> m_cbs;
    };
}