#include "PipelineBuilder.h"
#include "PipelineRunner.h"
#include <string>

int main() {
  using namespace analysis;

  AnalysisPluginHost analysis_host;
  PlotPluginHost plot_host;
  PipelineBuilder builder(analysis_host, plot_host);

  // Configure analysis region and at least one variable to satisfy pipeline requirements.
  builder.preset("QUALITY_NUMU_CC");
  builder.variable("TRUE_NEUTRINO_VERTEX",
                   {{"analysis_configs", {{"region", "QUALITY_NUMU_CC"}}}});

  // Generate a cut flow plot for the combined selection.
  builder.preset("CUT_FLOW_PLOT",
                 {{"plot_configs",
                   {{"selection_rule", "QUALITY_NUMU_CC"},
                    {"region", "QUALITY_NUMU_CC"},
                    {"signal_group", "inclusive_strange_channels"},
                    {"channel_column", "channel_definitions"},
                    {"initial_label", "All events"},
                    {"plot_name", "quality_numu_cc_cut_flow"}}}});

  auto analysis_specs = builder.analysisSpecs();
  auto plot_specs = builder.plotSpecs();

  PipelineRunner runner(analysis_specs, plot_specs);
  runner.run(std::string{"config/samples.json"},
             std::string{"/tmp/cut_flow.root"});

  return 0;
}

