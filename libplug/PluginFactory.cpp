#include <nlohmann/json.hpp>
#include "IAnalysisPlugin.h"

#ifndef PLUGIN_HEADER
#error "PLUGIN_HEADER not defined"
#endif
#include PLUGIN_HEADER

#ifndef PLUGIN_CLASS
#error "PLUGIN_CLASS not defined"
#endif

extern "C" analysis::IAnalysisPlugin *createPlugin(const nlohmann::json &cfg) {
    return new PLUGIN_CLASS(cfg);
}
