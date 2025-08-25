#include "RocCurvePlugin.h"
#include <nlohmann/json.hpp>

extern "C" analysis::IAnalysisPlugin* createPlugin(const nlohmann::json& cfg) {
    return new analysis::RocCurvePlugin(cfg);
}

extern "C" void setPluginContext(analysis::AnalysisDataLoader* loader) {
    analysis::RocCurvePlugin::setLoader(loader);
}
