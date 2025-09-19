#include "config_var_base.hpp"
#include <algorithm>

namespace sylar
{
    ConfigVarBase::ConfigVarBase(const std::string &name, const std::string &description)
        : m_name(name),
          m_description(description)
    {
        std::transform(m_name.begin(), m_name.end(), m_name.begin(), ::tolower);
    }

    ConfigVarBase::~ConfigVarBase()
    {
    }

    const std::string &ConfigVarBase::getName() const
    {
        return m_name;
    }

    const std::string &ConfigVarBase::getDescription() const
    {
        return m_description;
    }
}