#include "SnapshotPlugin.h"
#include <nlohmann/json.hpp>

extern "C" analysis::IAnalysisPlugin* createPlugin(const nlohmann::json& cfg) {
    return new analysis::SnapshotPlugin(cfg);
}

extern "C" void setPluginContext(analysis::AnalysisDataLoader* loader) {
    analysis::SnapshotPlugin::setLoader(loader);
}
