#include <nlohmann/json.hpp>

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

  // Minimal sample configuration
  nlohmann::json samples = {
      {"ntupledir", "/path/to/ntuples"},
      {"beamlines", {{"bnb", {{"run1", {}}}}}}};

  PipelineRunner runner(analysis_specs, plot_specs);
  runner.run(samples, "/tmp/output.root");

  return 0;
}
