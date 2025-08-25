#include "VariablesPlugin.h"
#include <nlohmann/json.hpp>

extern "C" analysis::IAnalysisPlugin *createPlugin(const nlohmann::json &cfg) {
    return new analysis::VariablesPlugin(cfg);
}