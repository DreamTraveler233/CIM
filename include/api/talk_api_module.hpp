#pragma once

#include "other/module.hpp"

namespace CIM::api {

class TalkApiModule : public CIM::Module {
   public:
    TalkApiModule();
    ~TalkApiModule() override = default;

    bool onServerReady() override;
};

}  // namespace CIM::api