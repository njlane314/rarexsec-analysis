#include "EventDisplayPlugin.h"
#include <nlohmann/json.hpp>

extern "C" analysis::IAnalysisPlugin *createPlugin(const nlohmann::json &cfg) {
    return new analysis::EventDisplayPlugin(cfg);
}

extern "C" void setPluginContext(analysis::AnalysisDataLoader *loader) {
    analysis::EventDisplayPlugin::setLoader(loader);
}