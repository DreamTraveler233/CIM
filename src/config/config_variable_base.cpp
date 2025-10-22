#include "config_variable_base.hpp"
#include "macro.hpp"
#include <algorithm>

namespace sylar
{
    ConfigVariableBase::ConfigVariableBase(const std::string &name, const std::string &description)
        : m_name(name),
          m_description(description)
    {
        SYLAR_ASSERT(!name.empty());
        std::transform(m_name.begin(), m_name.end(), m_name.begin(), ::tolower);
    }

    const std::string &ConfigVariableBase::getName() const
    {
        return m_name;
    }

    const std::string &ConfigVariableBase::getDescription() const
    {
        return m_description;
    }
}