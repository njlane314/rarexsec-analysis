#include "PresetRegistry.h"
#include "PluginSpec.h"
using namespace analysis;

// Baseline analysis preset
ANALYSIS_REGISTER_PRESET(BaselineAnalysis, Target::Analysis,
  [](const PluginArgs& vars) -> PluginSpecList {
    PluginArgs varCfg = {
      {"analysis_configs", {
        {"variables", vars.value("variables", PluginArgs::array({ "muon_pt", "pion_count" })) }
      }}
    };
    PluginArgs regCfg = {
      {"analysis_configs", {
        {"regions", vars.value("regions", PluginArgs::array({ "signal", "control" })) }
      }}
    };
    return {
      {"VariablesPlugin", varCfg},
      {"RegionsPlugin",   regCfg}
    };
  }
)

// Truth metrics preset
ANALYSIS_REGISTER_PRESET(TruthMetrics, Target::Analysis,
  [](const PluginArgs& vars) -> PluginSpecList {
    return {
      {"TruthMatchingPlugin", {{"threshold", vars.value("threshold", 0.5)}}},
      {"EfficiencyPlugin",    {{"by", "hits"}}}
    };
  }
)

// Plot preset
ANALYSIS_REGISTER_PRESET(StandardPlots, Target::Plot,
  [](const PluginArgs& vars) -> PluginSpecList {
    return {
      {"CutFlowPlot",     {{"plot_configs", {{"style", "stacked"}}}}},
      {"KinematicsPlots", {{"plot_configs", {{"pt_bins", vars.value("pt_bins", PluginArgs::array({0,5,10,20,40}))}}}}}
    };
  }
)
