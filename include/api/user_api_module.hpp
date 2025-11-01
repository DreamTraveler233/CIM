#pragma once

#include "other/module.hpp"

namespace CIM::api {

class UserApiModule : public CIM::Module {
   public:
    UserApiModule();
    ~UserApiModule() override = default;

    bool onServerReady() override;
};

}  // namespace CIM::api