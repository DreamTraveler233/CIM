#pragma once

#include "other/module.hpp"

namespace CIM::api {

class AuthApiModule : public CIM::Module {
   public:
    AuthApiModule();
    ~AuthApiModule() override = default;

    bool onServerReady() override;
};

}  // namespace CIM::api
