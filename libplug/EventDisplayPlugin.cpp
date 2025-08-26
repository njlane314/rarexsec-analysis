#include "EventDisplayPlugin.h"
#include <nlohmann/json.hpp>

// Factory function invoked by the plugin manager to create an instance of the
// EventDisplayPlugin.
extern "C" analysis::IAnalysisPlugin *createPlugin(const nlohmann::json &cfg) {
    return new analysis::EventDisplayPlugin(cfg);
}

// Optional context hook.  If present, AnalysisPluginManager calls this with a
// pointer to the long-lived AnalysisDataLoader so that the plugin can fetch
// event data during its finalisation step.
extern "C" void setPluginContext(analysis::AnalysisDataLoader *loader) {
    analysis::EventDisplayPlugin::setLoader(loader);
}