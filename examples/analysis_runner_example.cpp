#include <memory>
#include <nlohmann/json.hpp>
#include <vector>

#include "AnalysisDataLoader.h"
#include "AnalysisRunner.h"
#include "HistogramFactory.h"
#include "PipelineBuilder.h"
#include "PresetRegistry.h"
#include "RunConfigLoader.h"
#include "RunConfigRegistry.h"
#include "SystematicsProcessor.h"
#include "VariableRegistry.h"

// Demonstrates constructing plugin specifications programmatically and
// passing them directly to AnalysisRunner.
int main() {
  using namespace analysis;

  AnalysisPluginHost analysis_host;
  PlotPluginHost plot_host;
  PipelineBuilder builder(analysis_host, plot_host);

  builder.region("TRUE_NEUTRINO_VERTEX");
  builder.region("RECO_NEUTRINO_VERTEX");
  builder.variable("EMPTY");
  builder.preset("STACKED_PLOTS");
  builder.uniqueById();

  auto analysis_specs = builder.analysisSpecs();

  nlohmann::json samples = {{"ntupledir", "/path/to/ntuples"},
                            {"beamlines", {{"bnb", {{"run1", {}}}}}}};

  RunConfigRegistry run_config_registry;
  RunConfigLoader::loadFromJson(samples, run_config_registry);

  VariableRegistry variable_registry;
  SystematicsProcessor syst_processor(variable_registry);

  AnalysisDataLoader data_loader(run_config_registry, variable_registry, "bnb",
                                 {"run1"}, "/path/to/ntuples", true);

  auto histogram_factory = std::make_unique<HistogramFactory>();

  AnalysisRunner runner(data_loader, std::move(histogram_factory),
                        syst_processor, analysis_specs);
  auto result = runner.run();
  (void)result;
  return 0;
}
