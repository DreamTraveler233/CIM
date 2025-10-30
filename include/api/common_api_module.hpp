#pragma once

#include "other/module.hpp"

namespace CIM::api
{

    class CommonApiModule : public CIM::Module
    {
    public:
        CommonApiModule();
        ~CommonApiModule() override = default;

        bool onServerReady() override;
    };

} // namespace CIM::api
