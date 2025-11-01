#pragma once

#include "other/module.hpp"

namespace CIM::api {

class ArticleApiModule : public CIM::Module {
   public:
    ArticleApiModule();
    ~ArticleApiModule() override = default;

    bool onServerReady() override;
};

}  // namespace CIM::api