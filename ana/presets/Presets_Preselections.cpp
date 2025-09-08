#include "PluginSpec.h"
#include "PresetRegistry.h"
#include <nlohmann/json.hpp>

using namespace analysis;

// Preset defining basic variable preselections for quality checks.
// Allows an optional `region` override via PluginArgs.
ANALYSIS_REGISTER_PRESET(
    PRESELECTION_VARIABLES, Target::Analysis,
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
            {"bins", {{"mode", "dynamic"}, {"strategy", "bayesian_blocks"}}}},
           {{"name", "neutrino_energy"},
            {"branch", "neutrino_energy"},
            {"label", "Neutrino energy [GeV]"},
            {"stratum", "channel_definitions"},
            {"regions", regions},
            {"bins", {{"mode", "dynamic"}, {"strategy", "bayesian_blocks"}}}},
           {{"name", "optical_filter_pe_beam"},
            {"branch", "optical_filter_pe_beam"},
            {"label", "Optical filter beam PE"},
            {"stratum", "channel_definitions"},
            {"regions", regions},
            {"bins", {{"mode", "dynamic"}, {"strategy", "bayesian_blocks"}}}},
           {{"name", "num_slices"},
            {"branch", "num_slices"},
            {"label", "Number of slices"},
            {"stratum", "channel_definitions"},
            {"regions", regions},
            {"bins", {{"mode", "dynamic"}, {"strategy", "bayesian_blocks"}}}},
           {{"name", "software_trigger"},
            {"branch", "software_trigger"},
            {"label", "Software trigger"},
            {"stratum", "channel_definitions"},
            {"regions", regions},
            {"bins", {{"mode", "dynamic"}, {"strategy", "bayesian_blocks"}}}},
           {{"name", "slice_cluster_fraction"},
            {"branch", "slice_cluster_fraction"},
            {"label", "Fraction of slice clustered"},
            {"stratum", "channel_definitions"},
            {"regions", regions},
            {"bins", {{"mode", "dynamic"}, {"strategy", "bayesian_blocks"}}}},
           {{"name", "slice_contained_fraction"},
            {"branch", "slice_contained_fraction"},
            {"label", "Fraction of slice contained"},
            {"stratum", "channel_definitions"},
            {"regions", regions},
            {"bins", {{"mode", "dynamic"}, {"strategy", "bayesian_blocks"}}}}});

      PluginArgs args{{"analysis_configs", {{"variables", var_defs}}}};
      return {{"VariablesPlugin", args}};
    }));
