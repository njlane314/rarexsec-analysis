#include "OccupancyMatrixPlugin.h"
#include <nlohmann/json.hpp>

extern "C" analysis::IAnalysisPlugin *createPlugin(const nlohmann::json &cfg) {
    return new analysis::OccupancyMatrixPlugin(cfg);
}

extern "C" void setPluginContext(analysis::AnalysisDataLoader *loader) {
    analysis::OccupancyMatrixPlugin::setLoader(loader);
}
