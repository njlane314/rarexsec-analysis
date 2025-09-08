#include "PresetRegistry.h"
#include "PluginSpec.h"
#include <nlohmann/json.hpp>

using namespace analysis;

// Placeholder preset registrations.  The detailed JSON-driven implementations
// were removed as PluginArgs now exposes explicit fields and validation occurs
// in plugin code.  These presets can be expanded as needed by providing
// PluginSpecLists constructed from the strongly typed PluginArgs structure.
ANALYSIS_REGISTER_PRESET(BaselineAnalysis, Target::Analysis,
  [](const PluginArgs&) -> PluginSpecList { return {}; }
)

ANALYSIS_REGISTER_PRESET(TruthMetrics, Target::Analysis,
  [](const PluginArgs&) -> PluginSpecList { return {}; }
)

ANALYSIS_REGISTER_PRESET(StandardPlots, Target::Plot,
  [](const PluginArgs&) -> PluginSpecList { return {}; }
)

// Presets defining common analysis regions.  Each preset configures the
// RegionsPlugin with a single region tied to a selection rule.
ANALYSIS_REGISTER_PRESET(EMPTY, Target::Analysis,
  [](const PluginArgs&) -> PluginSpecList {
    nlohmann::json region = {
      {"region_key", "EMPTY"},
      {"label", "Empty Selection"},
      {"selection_rule", "NONE"}
    };
    PluginArgs args{{"analysis_configs", {{"regions", nlohmann::json::array({region})}}}};
    return {{"RegionsPlugin", args}};
  }
)

ANALYSIS_REGISTER_PRESET(QUALITY, Target::Analysis,
  [](const PluginArgs&) -> PluginSpecList {
    nlohmann::json region = {
      {"region_key", "QUALITY"},
      {"label", "Quality Selection"},
      {"selection_rule", "QUALITY"}
    };
    PluginArgs args{{"analysis_configs", {{"regions", nlohmann::json::array({region})}}}};
    return {{"RegionsPlugin", args}};
  }
)

ANALYSIS_REGISTER_PRESET(MUON, Target::Analysis,
  [](const PluginArgs&) -> PluginSpecList {
    nlohmann::json region = {
      {"region_key", "MUON"},
      {"label", "Muon Selection"},
      {"selection_rule", "MUON"}
    };
    PluginArgs args{{"analysis_configs", {{"regions", nlohmann::json::array({region})}}}};
    return {{"RegionsPlugin", args}};
  }
)

