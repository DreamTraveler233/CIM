#include "app/common_service.hpp"

#include "macro.hpp"

namespace CIM::app {
std::string CommonService::SendSmsCode() {
    // 生成6位数字验证码
    std::string sms_code = CIM::random_string(6, "0123456789");
    return sms_code;
}
}  // namespace CIM::app
