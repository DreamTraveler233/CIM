#pragma once

#include "other/module.hpp"

namespace CIM::api {

class GroupApiModule : public CIM::Module {
   public:
    GroupApiModule();
    ~GroupApiModule() override = default;

    bool onServerReady() override;
};

}  // namespace CIM::api