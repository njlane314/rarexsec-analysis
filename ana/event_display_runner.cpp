#include "PipelineBuilder.h"
#include "PipelineRunner.h"
#include <string>

int main() {
  using namespace analysis;

  AnalysisPluginHost analysis_host;
  PlotPluginHost plot_host;
  PipelineBuilder builder(analysis_host, plot_host);

  // Restrict to a predefined analysis region and include at least one variable
  // definition to satisfy pipeline requirements.
  builder.region("QUALITY");
  // Attach vertex variables to the QUALITY region so the preset does not
  // default to the placeholder region and cause initialisation failures.
  builder.variable("TRUE_NEUTRINO_VERTEX",
                   { {"analysis_configs", { {"region", "QUALITY"} }} });

  // Generate event displays for background and signal samples.  The presets
  // provide sensible defaults for sample selection and output locations.
  builder.preset("BACKGROUND_EVENT_DISPLAY",
                 { {"plot_configs", {{"region", "QUALITY"}, {"n_events", 2}}} });
  builder.preset("SIGNAL_EVENT_DISPLAY",
                 { {"plot_configs", {{"region", "QUALITY"}, {"n_events", 2}}} });
  //builder.uniqueById();

  auto analysis_specs = builder.analysisSpecs();
  auto plot_specs = builder.plotSpecs();

  PipelineRunner runner(analysis_specs, plot_specs);
  runner.run(std::string{"config/samples.json"},
             std::string{"/tmp/event_displays.root"});

  return 0;
}
