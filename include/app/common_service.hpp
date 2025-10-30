#pragma once

#include <string>

namespace CIM::app {
class CommonService {
   public:
    // 短信服务
    static std::string SendSmsCode();
};
}  // namespace CIM::app