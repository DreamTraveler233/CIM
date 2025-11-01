#pragma once

#include "other/module.hpp"

namespace CIM::api {

class ImApiModule : public CIM::Module {
   public:
    ImApiModule();
    ~ImApiModule() override = default;

    bool onServerReady() override;
};

}  // namespace CIM::api
