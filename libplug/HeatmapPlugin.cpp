#include "HeatmapPlugin.h"
#include <nlohmann/json.hpp>

extern "C" analysis::IAnalysisPlugin *createPlugin(const nlohmann::json &cfg) {
    return new analysis::HeatmapPlugin(cfg);
}

extern "C" void setPluginContext(analysis::AnalysisDataLoader *loader) {
    analysis::HeatmapPlugin::setLoader(loader);
}
