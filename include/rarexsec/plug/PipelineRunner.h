#pragma once

#include <algorithm>
#include <fstream>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <ROOT/RDataFrame.hxx>
#include <nlohmann/json.hpp>

#include <rarexsec/data/AnalysisDataLoader.h>
#include <rarexsec/app/AnalysisResult.h>
#include <rarexsec/app/AnalysisRunner.h>
#include <rarexsec/hist/HistogramFactory.h>
#include <rarexsec/utils/Logger.h>
#include <rarexsec/plug/PluginAliases.h>
#include <rarexsec/plug/PluginSpec.h>
#include <rarexsec/data/RunConfigLoader.h>
#include <rarexsec/data/RunConfigRegistry.h>
#include <rarexsec/syst/SystematicsProcessor.h>
#include <rarexsec/data/VariableRegistry.h>

namespace analysis {

namespace detail {

inline AnalysisResult processBeamline(
    RunConfigRegistry &run_config_registry, const std::string &ntuple_dir,
    const std::string &beam, const nlohmann::json &runs,
    const PluginSpecList &analysis_specs,
    const PluginSpecList &syst_specs) {
  std::vector<std::string> periods;
  periods.reserve(runs.size());
  for (auto const &[period, _] : runs.items())
    periods.emplace_back(period);

  VariableRegistry variable_registry;
  SystematicsProcessor systematics_processor(variable_registry);
  AnalysisDataLoader data_loader(run_config_registry, variable_registry, beam,
                                 periods, ntuple_dir, true);
  auto histogram_factory = std::make_unique<HistogramFactory>();

  AnalysisRunner runner(data_loader, std::move(histogram_factory),
                        systematics_processor, analysis_specs, syst_specs);
  auto result = runner.run();

  for (auto &kv : result.regions()) {
    if (kv.second.beamConfig().empty()) {
      kv.second.setBeamConfig(beam);
      kv.second.setRunNumbers(periods);
    }
  }

  return result;
}

inline void aggregateResults(AnalysisResult &result,
                             const AnalysisResult &beamline_result) {
  for (auto &kv : beamline_result.regions())
    result.regions().insert(kv);
}

inline AnalysisResult runAnalysis(const nlohmann::json &samples,
                                  const PluginSpecList &analysis_specs,
                                  const PluginSpecList &syst_specs) {
  ROOT::EnableImplicitMT();
  log::info("analysis::runAnalysis", "Implicit multithreading engaged across",
            ROOT::GetThreadPoolSize(), "threads.");

  std::string ntuple_dir = samples.at("ntupledir").get<std::string>();
  log::info("analysis::runAnalysis", "Configuration loaded for",
            samples.at("beamlines").size(), "beamlines.");

  RunConfigRegistry run_config_registry;
  RunConfigLoader::loadFromJson(samples, run_config_registry);

  AnalysisResult result;
  for (auto const &[beam, runs] : samples.at("beamlines").items()) {
    if (beam == "numi_ext")
      continue;
    auto beamline_result =
        processBeamline(run_config_registry, ntuple_dir, beam, runs,
                        analysis_specs, syst_specs);
    aggregateResults(result, beamline_result);
  }

  return result;
}

inline void plotBeamline(RunConfigRegistry &run_config_registry,
                         const std::string &ntuple_dir,
                         const std::string &beam, const nlohmann::json &runs,
                         const PluginSpecList &plot_specs,
                         const AnalysisResult &beam_result) {
  std::vector<std::string> periods;
  periods.reserve(runs.size());
  for (auto const &[period, _] : runs.items())
    periods.emplace_back(period);

  VariableRegistry variable_registry;
  AnalysisDataLoader data_loader(run_config_registry, variable_registry, beam,
                                 periods, ntuple_dir, true);

  PlotPluginHost p_host(&data_loader);
  for (auto const &spec : plot_specs)
    p_host.add(spec.id, spec.args);

  p_host.forEach([&](IPlotPlugin &pl) { pl.onPlot(beam_result); });
}

inline void runPlotting(const nlohmann::json &samples,
                        const PluginSpecList &plot_specs,
                        const AnalysisResult &result) {
  std::string ntuple_dir = samples.at("ntupledir").get<std::string>();
  log::info("analysis::runPlotting", "Configuration loaded for",
            samples.at("beamlines").size(), "beamlines.");

  RunConfigRegistry run_config_registry;
  RunConfigLoader::loadFromJson(samples, run_config_registry);

  auto result_map = result.resultsByBeam();
  bool plotted = false;
  for (auto const &[beam, runs] : samples.at("beamlines").items()) {
    if (beam == "numi_ext")
      continue;
    auto it = result_map.find(beam);
    if (it != result_map.end()) {
      plotBeamline(run_config_registry, ntuple_dir, beam, runs, plot_specs,
                   it->second);
      plotted = true;
    }
  }

  if (!plotted) {
    PlotPluginHost p_host; // No data loader context available
    for (auto const &spec : plot_specs)
      p_host.add(spec.id, spec.args);
    p_host.forEach([&](IPlotPlugin &pl) { pl.onPlot(result); });
  }

  log::info("analysis::runPlotting", "Plotting routine terminated nominally.");
}

} // namespace detail

// Helper function to build analysis and plot pipelines from a JSON
// description. The returned pair contains analysis plugin specs and
// plot plugin specs respectively.
// PipelineRunner orchestrates the execution of the analysis and optional
// plotting stages once a pipeline has been constructed.
class PipelineRunner {
public:
  PipelineRunner(PluginSpecList analysis_specs,
                 PluginSpecList plot_specs,
                 PluginSpecList systematics_specs = {})
      : analysis_specs_(std::move(analysis_specs)),
        plot_specs_(std::move(plot_specs)),
        systematics_specs_(std::move(systematics_specs)) {}

  // Execute the analysis and plotting for the provided samples
  // configuration. The analysis result is written to \p output_path and
  // returned to the caller.
  inline AnalysisResult run(const nlohmann::json &samples,
                            const std::string /*&output_path*/) const {
    auto result = detail::runAnalysis(samples, analysis_specs_, systematics_specs_);
    //result.saveToFile(output_path.c_str());
    detail::runPlotting(samples, plot_specs_, result);
    return result;
  }

  // Convenience overload that reads the samples configuration from a JSON
  // file located at \p samples_path before executing the pipeline.
  inline AnalysisResult run(const std::string &samples_path,
                            const std::string &output_path) const {
    std::ifstream in(samples_path);
    nlohmann::json samples;
    in >> samples;
    if (samples.contains("samples"))
      samples = samples.at("samples");
    return run(samples, output_path);
  }

private:
  PluginSpecList analysis_specs_;
  PluginSpecList plot_specs_;
  PluginSpecList systematics_specs_;
};

} // namespace analysis

