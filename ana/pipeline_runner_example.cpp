#include "PipelineBuilder.h"
#include "PipelineRunner.h"

int main() {
  using namespace analysis;

  AnalysisPluginHost analysis_host;
  PlotPluginHost plot_host;
  PipelineBuilder builder(analysis_host, plot_host);

  // Build a simple pipeline programmatically
  builder.region("TRUE_NEUTRINO_VERTEX");
  builder.region("RECO_NEUTRINO_VERTEX");
  builder.variable("EMPTY");
  builder.preset("STACKED_PLOTS");
  builder.uniqueById();

  auto analysis_specs = builder.analysisSpecs();
  auto plot_specs = builder.plotSpecs();

  PipelineRunner runner(analysis_specs, plot_specs);
  runner.run("config/samples.json", "/tmp/output.root");

  return 0;
}
