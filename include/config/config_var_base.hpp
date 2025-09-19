#pragma once

#include <string>
#include <memory>

namespace sylar
{
    class ConfigVarBase
    {
    public:
        using ptr = std::shared_ptr<ConfigVarBase>;

        ConfigVarBase(const std::string &name, const std::string &description = "");
        virtual ~ConfigVarBase();
        virtual std::string toString() = 0;
        virtual bool fromString(const std::string &val) = 0;
        virtual std::string getTypeName() const = 0;
        const std::string &getName() const;
        const std::string &getDescription() const;

    private:
        std::string m_name;        // 配置项名称
        std::string m_description; // 配置项描述
    };
}