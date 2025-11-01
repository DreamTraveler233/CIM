#pragma once

#include "other/module.hpp"

namespace CIM::api {

class WsGatewayModule : public CIM::Module {
   public:
    WsGatewayModule();
    ~WsGatewayModule() override = default;

    bool onServerReady() override;
};

}  // namespace CIM::api
