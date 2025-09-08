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

// Preset to configure the EventDisplay plugin with a single display request.
// Values are taken from the provided variables or fall back to sensible
// defaults matching the plugin's own choices.
ANALYSIS_REGISTER_PRESET(EVENT_DISPLAY, Target::Plot,
  [](const PluginArgs& vars) -> PluginSpecList {
    auto cfg = vars.plot_configs;
    nlohmann::json ed = {
      {"sample", cfg.value("sample", std::string{})},
      {"region", cfg.value("region", std::string{})},
      {"n_events", cfg.value("n_events", 1)},
      {"image_size", cfg.value("image_size", 800)},
      {"output_directory",
       cfg.value("output_directory", std::string{"./plots/event_displays"})}
    };
    PluginArgs args{{"plot_configs",
                     {{"event_displays", nlohmann::json::array({ed})}}}};
    return {{"EventDisplayPlugin", args}};
  }
)

