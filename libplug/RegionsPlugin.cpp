#include "RegionsPlugin.h"
#include <nlohmann/json.hpp>

extern "C" analysis::IAnalysisPlugin *createRegionsPlugin(const nlohmann::json &cfg) {
    return new analysis::RegionsPlugin(cfg);
}

