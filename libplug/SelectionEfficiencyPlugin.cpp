#include "SelectionEfficiencyPlugin.h"
#include <nlohmann/json.hpp>

extern "C" analysis::IAnalysisPlugin *createPlugin(const nlohmann::json &cfg) {
    return new analysis::SelectionEfficiencyPlugin(cfg);
}

extern "C" void setPluginContext(analysis::AnalysisDataLoader *loader) {
    analysis::SelectionEfficiencyPlugin::setLoader(loader);
}
