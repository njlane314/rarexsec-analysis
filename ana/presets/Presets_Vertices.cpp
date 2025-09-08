#include "PluginSpec.h"
#include "PresetRegistry.h"
#include <nlohmann/json.hpp>

using namespace analysis;

// Preset defining variables for the true neutrino interaction vertex.
//
// The region to which the variables are attached can be overridden via the
// `analysis_configs` field with a key named `region`.  If no override is
// provided the variables default to the placeholder `EMPTY` region.  This keeps
// the preset usable in minimal pipelines while allowing callers to target a
// specific analysis region.
ANALYSIS_REGISTER_PRESET(TRUE_NEUTRINO_VERTEX, Target::Analysis,
                         ([](const PluginArgs &vars) -> PluginSpecList {
                           std::string region =
                               vars.analysis_configs.value("region",
                                                           std::string{"EMPTY"});
                           auto regions = PluginArgs::array({region});

                           nlohmann::json var_defs = PluginArgs::array({
                               {{"name", "nu_vtx_x"},
                                {"branch", "neutrino_vertex_x"},
                                {"label", "True #nu Vertex X [cm]"},
                                {"stratum", "inclusive_strange_channels"},
                                {"regions", regions},
                                {"bins", {{"mode", "dynamic"},
                                           {"strategy", "bayesian_blocks"},
                                           {"resolution", 0.3}}}},
                               {{"name", "nu_vtx_y"},
                                {"branch", "neutrino_vertex_y"},
                                {"label", "True #nu Vertex Y [cm]"},
                                {"stratum", "inclusive_strange_channels"},
                                {"regions", regions},
                                {"bins", {{"mode", "dynamic"},
                                           {"strategy", "bayesian_blocks"},
                                           {"resolution", 0.3}}}},
                               {{"name", "nu_vtx_z"},
                                {"branch", "neutrino_vertex_z"},
                                {"label", "True #nu Vertex Z [cm]"},
                                {"stratum", "inclusive_strange_channels"},
                                {"regions", regions},
                                {"bins", {{"mode", "dynamic"},
                                           {"strategy", "bayesian_blocks"},
                                           {"resolution", 0.3}}}}});

                           PluginArgs args{{"analysis_configs",
                                            {{"variables", var_defs}}}};
                           return {{"VariablesPlugin", args}};
                         }))

// Preset defining variables for the reconstructed neutrino interaction vertex.
// Behaves like TRUE_NEUTRINO_VERTEX with support for an optional `region`
// override via PluginArgs.
ANALYSIS_REGISTER_PRESET(RECO_NEUTRINO_VERTEX, Target::Analysis,
                         ([](const PluginArgs &vars) -> PluginSpecList {
                           std::string region =
                               vars.analysis_configs.value("region",
                                                           std::string{"EMPTY"});
                           auto regions = PluginArgs::array({region});

                           nlohmann::json var_defs = PluginArgs::array({
                               {{"name", "reco_nu_vtx_x"},
                                {"branch", "reco_neutrino_vertex_x"},
                                {"label", "Reco #nu Vertex X [cm]"},
                                {"stratum", "inclusive_strange_channels"},
                                {"regions", regions},
                                {"bins", {{"mode", "dynamic"},
                                           {"include_oob_bins", true},
                                           {"strategy", "bayesian_blocks"},
                                           {"resolution", 0.3}}}},
                               {{"name", "reco_nu_vtx_y"},
                                {"branch", "reco_neutrino_vertex_y"},
                                {"label", "Reco #nu Vertex Y [cm]"},
                                {"stratum", "inclusive_strange_channels"},
                                {"regions", regions},
                                {"bins", {{"mode", "dynamic"},
                                           {"strategy", "bayesian_blocks"},
                                           {"resolution", 0.3}}}},
                               {{"name", "reco_nu_vtx_z"},
                                {"branch", "reco_neutrino_vertex_z"},
                                {"label", "Reco #nu Vertex Z [cm]"},
                                {"stratum", "inclusive_strange_channels"},
                                {"regions", regions},
                                {"bins", {{"mode", "dynamic"},
                                           {"strategy", "bayesian_blocks"},
                                           {"resolution", 0.3}}}}});

                           PluginArgs args{{"analysis_configs",
                                            {{"variables", var_defs}}}};
                           return {{"VariablesPlugin", args}};
                         }))
