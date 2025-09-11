#include "PluginSpec.h"
#include "PresetRegistry.h"
#include <nlohmann/json.hpp>

using namespace analysis;

// Preset defining only the topological score variable with fixed bins for
// testing. Updated to use 100 bins across the 0 to 1 range.
ANALYSIS_REGISTER_PRESET(
    TEST_TOPOLOGICAL_SCORE, Target::Analysis,
    ([](const PluginArgs &vars) -> PluginSpecList {
      std::string region =
          vars.analysis_configs.value("region", std::string{"EMPTY"});
      auto regions = PluginArgs::array({region});

      nlohmann::json var_defs = PluginArgs::array(
          {{{"name", "topological_score"},
            {"branch", "topological_score"},
            {"label", "Topological score"},
            {"stratum", "channel_definitions"},
            {"regions", regions},
            // Use 100 bins between 0 and 1.
            {"bins", {{"n", 100}, {"min", 0.0}, {"max", 1.0}}}}});

      PluginArgs args{{"analysis_configs", {{"variables", var_defs}}}};
      return {{"VariablesPlugin", args}};
    }));
