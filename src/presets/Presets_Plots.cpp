#include "PresetRegistry.h"
#include "PluginSpec.h"
#include <nlohmann/json.hpp>

using namespace analysis;

// Presets for plot-specific configuration.
// Configures stacked histogram plots stratified by the inclusive category scheme.
ANALYSIS_REGISTER_PRESET(STACKED_PLOTS, Target::Plot,
  [](const PluginArgs&) -> PluginSpecList {
    nlohmann::json plot = {{"category_column", "inclusive"}};
    PluginArgs args{{"plot_configs", {{"plots", nlohmann::json::array({plot})}}}};
    return {{"StackedHistogramPlugin", args}};
  }
)

