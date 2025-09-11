#pragma once
#include "SystematicsProcessor.h"

namespace analysis {

class ISystematicsPlugin {
public:
    virtual ~ISystematicsPlugin() = default;
    virtual void configure(SystematicsProcessor& proc) = 0;
};

} // namespace analysis
