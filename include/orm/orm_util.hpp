#ifndef __CIM_ORM_ORM_UTIL_HPP__
#define __CIM_ORM_ORM_UTIL_HPP__

#include <tinyxml2.h>
#include <string>

namespace CIM::orm
{
    std::string GetAsVariable(const std::string &v);
    std::string GetAsClassName(const std::string &v);
    std::string GetAsMemberName(const std::string &v);
    std::string GetAsGetFunName(const std::string &v);
    std::string GetAsSetFunName(const std::string &v);
    std::string XmlToString(const tinyxml2::XMLNode &node);
    std::string GetAsDefineMacro(const std::string &v);

}

#endif