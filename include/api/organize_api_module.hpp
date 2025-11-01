#pragma once

#include "other/module.hpp"

namespace CIM::api {

class OrganizeApiModule : public CIM::Module {
   public:
    OrganizeApiModule();
    ~OrganizeApiModule() override = default;

    bool onServerReady() override;
};

}  // namespace CIM::api