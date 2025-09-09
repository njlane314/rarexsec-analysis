#include "PipelineBuilder.h"
#include "PipelineRunner.h"
#include <string>

int main() {
  using namespace analysis;

  AnalysisPluginHost analysis_host;
  PlotPluginHost plot_host;
  PipelineBuilder builder(analysis_host, plot_host);

  // Build a simple pipeline programmatically
  builder.region("EMPTY");
  builder.variable("TEST_TOPOLOGICAL_SCORE");
  // Configure a performance plot for the topological score
  builder.add(
      Target::Plot, "PerformancePlotPlugin",
      {{"plot_configs",
        {{"performance_plots",
          PluginArgs::array({{{"region", "EMPTY"},
                               {"channel_column", "channel_definitions"},
                               {"signal_group", "inclusive_strange_channels"},
                               {"variable", "topological_score"},
                               {"plot_name", "topological_score_performance"},
                               {"output_directory", "./plots"}}})}}}});

  auto analysis_specs = builder.analysisSpecs();
  auto plot_specs = builder.plotSpecs();

  PipelineRunner runner(analysis_specs, plot_specs);
  runner.run(std::string{"config/samples.json"}, std::string{"/tmp/output.root"});

  return 0;
}
