#pragma once

#include "other/module.hpp"

namespace CIM::api {

class EmoticonApiModule : public CIM::Module {
   public:
    EmoticonApiModule();
    ~EmoticonApiModule() override = default;

    bool onServerReady() override;
};

}  // namespace CIM::api