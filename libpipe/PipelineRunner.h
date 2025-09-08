#pragma once

#include <string>
#include <utility>
#include <nlohmann/json.hpp>

#include "AnalysisResult.h"
#include "PluginSpec.h"

namespace analysis {

// Helper function to build analysis and plot pipelines from a JSON
// description. The returned pair contains analysis plugin specs and
// plot plugin specs respectively.
std::pair<PluginSpecList, PluginSpecList>
buildPipeline(const nlohmann::json &cfg);

// PipelineRunner orchestrates the execution of the analysis and optional
// plotting stages once a pipeline has been constructed.
class PipelineRunner {
public:
  PipelineRunner(PluginSpecList analysis_specs,
                 PluginSpecList plot_specs);

  // Execute the analysis and plotting for the provided samples
  // configuration. The analysis result is written to \p output_path and
  // returned to the caller.
  AnalysisResult run(const nlohmann::json &samples,
                     const std::string &output_path) const;

private:
  PluginSpecList analysis_specs_;
  PluginSpecList plot_specs_;
};

} // namespace analysis

