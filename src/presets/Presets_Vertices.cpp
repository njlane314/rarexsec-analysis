#include "PresetRegistry.h"
#include "PluginSpec.h"
#include <nlohmann/json.hpp>

using namespace analysis;

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

// Preset defining variables for the reconstructed neutrino interaction vertex.
ANALYSIS_REGISTER_PRESET(RECO_NEUTRINO_VERTEX, Target::Analysis,
  [](const PluginArgs&) -> PluginSpecList {
    nlohmann::json vars = nlohmann::json::array({
      {
        {"name", "reco_nu_vtx_x"},
        {"branch", "reco_neutrino_vertex_x"},
        {"label", "Reco #nu Vertex X [cm]"},
        {"stratum", "event"},
        {"bins", {{"n", 26}, {"min", 0.0}, {"max", 260.0}}}
      },
      {
        {"name", "reco_nu_vtx_y"},
        {"branch", "reco_neutrino_vertex_y"},
        {"label", "Reco #nu Vertex Y [cm]"},
        {"stratum", "event"},
        {"bins", {{"n", 24}, {"min", -120.0}, {"max", 120.0}}}
      },
      {
        {"name", "reco_nu_vtx_z"},
        {"branch", "reco_neutrino_vertex_z"},
        {"label", "Reco #nu Vertex Z [cm]"},
        {"stratum", "event"},
        {"bins", {{"n", 52}, {"min", 0.0}, {"max", 1040.0}}}
      }
    });
    PluginArgs args{{"analysis_configs", {{"variables", vars}}}};
    return {{"VariablesPlugin", args}};
  }
)



