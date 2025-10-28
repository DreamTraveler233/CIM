#pragma once

#include "other/module.hpp"

namespace CIM::api
{

    class MinimalApiModule : public CIM::Module
    {
    public:
        MinimalApiModule();
        ~MinimalApiModule() override = default;

        bool onServerReady() override;
    };

} // namespace CIM::api
