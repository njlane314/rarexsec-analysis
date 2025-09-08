#include "PresetRegistry.h"
#include "PluginSpec.h"

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
