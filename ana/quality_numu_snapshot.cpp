#include "PipelineBuilder.h"
#include "PipelineRunner.h"
#include <string>

int main() {
  using namespace analysis;

  AnalysisPluginHost analysis_host;
  PlotPluginHost plot_host;
  PipelineBuilder builder(analysis_host, plot_host);

  // Apply quality and νμ CC selection presets then snapshot the resulting data frame.
  builder.variable("TRUE_NEUTRINO_VERTEX");
  builder.preset("QUALITY");
  builder.preset("NUMU_CC");
  builder.preset("NUMU_CC_SNAPSHOT");
  //builder.uniqueById();

  auto analysis_specs = builder.analysisSpecs();
  auto plot_specs = builder.plotSpecs();

  PipelineRunner runner(analysis_specs, plot_specs);
  runner.run(std::string{"config/samples.json"},
             std::string{"/tmp/numu_cc_snapshot.root"});

  return 0;
}
