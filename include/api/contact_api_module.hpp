#pragma once

#include "other/module.hpp"

namespace CIM::api {

class ContactApiModule : public CIM::Module {
   public:
    ContactApiModule();
    ~ContactApiModule() override = default;

    bool onServerReady() override;
};

}  // namespace CIM::api