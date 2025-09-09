#include "PipelineBuilder.h"
#include "PipelineRunner.h"
#include <string>

int main() {
  using namespace analysis;

  AnalysisPluginHost analysis_host;
  PlotPluginHost plot_host;
  PipelineBuilder builder(analysis_host, plot_host);

  // Build a pipeline examining the topological score before its quality cut
  builder.region("EMPTY",
                 {{"analysis_configs",
                   {{"regions",
                     PluginArgs::array({{{"region_key", "PRE_TOPO"},
                                          {"label", "Pre-topo quality"},
                                          {"expression",
                                           "in_reco_fiducial && num_slices == 1 && optical_filter_pe_beam > 20"}}})}}}});

  // Register the topological score variable in that region
  builder.variable("TEST_TOPOLOGICAL_SCORE",
                   {{"analysis_configs", {{"region", "PRE_TOPO"}}}});

  // Produce a stacked histogram with a logarithmic y-axis
  builder.preset("STACKED_PLOTS_LOG",
                 {{"plot_configs",
                   {{"plots",
                     PluginArgs::array({{{"variable", "topological_score"},
                                          {"region", "PRE_TOPO"},
                                          {"signal_group",
                                           "inclusive_strange_channels"}}})}}}});

  // Plot the ROC curve and significance improvement for the topological score
  builder.add(Target::Plot, "PerformancePlotPlugin",
              {{"performance_plots",
                PluginArgs::array({{{"region", "PRE_TOPO"},
                                     {"channel_column", "incl_channel"},
                                     {"signal_group",
                                      "inclusive_strange_channels"},
                                     {"variable", "topological_score"}}})}});

  auto analysis_specs = builder.analysisSpecs();
  auto plot_specs = builder.plotSpecs();

  PipelineRunner runner(analysis_specs, plot_specs);
  runner.run(std::string{"config/samples.json"}, std::string{"/tmp/output.root"});

  return 0;
}
