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

// Preset defining variables for the true neutrino interaction vertex.
ANALYSIS_REGISTER_PRESET(TRUE_NEUTRINO_VERTEX, Target::Analysis,
  [](const PluginArgs&) -> PluginSpecList {
    nlohmann::json vars = nlohmann::json::array({
      {
        {"name", "nu_vtx_x"},
        {"branch", "neutrino_vertex_x"},
        {"label", "True #nu Vertex X [cm]"},
        {"stratum", "event"},
        {"bins", {{"n", 26}, {"min", 0.0}, {"max", 260.0}}}
      },
      {
        {"name", "nu_vtx_y"},
        {"branch", "neutrino_vertex_y"},
        {"label", "True #nu Vertex Y [cm]"},
        {"stratum", "event"},
        {"bins", {{"n", 24}, {"min", -120.0}, {"max", 120.0}}}
      },
      {
        {"name", "nu_vtx_z"},
        {"branch", "neutrino_vertex_z"},
        {"label", "True #nu Vertex Z [cm]"},
        {"stratum", "event"},
        {"bins", {{"n", 52}, {"min", 0.0}, {"max", 1040.0}}}
      }
    });
    PluginArgs args{{"analysis_configs", {{"variables", vars}}}};
    return {{"VariablesPlugin", args}};
  }
)
