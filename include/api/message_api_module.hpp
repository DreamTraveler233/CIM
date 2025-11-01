#pragma once

#include "other/module.hpp"

namespace CIM::api {

class MessageApiModule : public CIM::Module {
   public:
    MessageApiModule();
    ~MessageApiModule() override = default;

    bool onServerReady() override;
};

}  // namespace CIM::api