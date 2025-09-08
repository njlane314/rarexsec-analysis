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

#include "AnalysisDataLoader.h"
#include "AnalysisResult.h"
#include "AnalysisRunner.h"
#include "HistogramFactory.h"
#include "Logger.h"
#include "PipelineBuilder.h"
#include "PluginAliases.h"
#include "PluginSpec.h"
#include "RunConfigLoader.h"
#include "RunConfigRegistry.h"
#include "SystematicsProcessor.h"
#include "VariableRegistry.h"

namespace analysis {

namespace detail {

inline AnalysisResult processBeamline(
    RunConfigRegistry &run_config_registry, const std::string &ntuple_dir,
    const std::string &beam, const nlohmann::json &runs,
    const PluginSpecList &analysis_specs) {
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
                        systematics_processor, analysis_specs);

  return runner.run();
}

inline void aggregateResults(AnalysisResult &result,
                             const AnalysisResult &beamline_result) {
  for (auto &kv : beamline_result.regions())
    result.regions().insert(kv);
}

inline AnalysisResult runAnalysis(const nlohmann::json &samples,
                                  const PluginSpecList &analysis_specs) {
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
    auto beamline_result =
        processBeamline(run_config_registry, ntuple_dir, beam, runs,
                        analysis_specs);
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
inline std::pair<PluginSpecList, PluginSpecList>
buildPipeline(const nlohmann::json &cfg) {
  AnalysisPluginHost a_host; // dummy hosts
  PlotPluginHost p_host;
  PipelineBuilder builder(a_host, p_host);

  if (cfg.contains("presets")) {
    for (auto const &p : cfg.at("presets")) {
      std::string name = p.at("name").get<std::string>();
      PluginArgs vars;
      if (p.contains("vars")) {
        const auto &vj = p.at("vars");
        vars = PluginArgs{{"analysis_configs",
                            vj.value("analysis_configs", nlohmann::json::object())},
                           {"plot_configs",
                            vj.value("plot_configs", nlohmann::json::object())}};
      }
      std::unordered_map<std::string, PluginArgs> overrides;
      if (p.contains("overrides")) {
        for (auto const &[k, v] : p.at("overrides").items()) {
          overrides.emplace(k, PluginArgs{{"analysis_configs",
                                           v.value("analysis_configs",
                                                   nlohmann::json::object())},
                                          {"plot_configs",
                                           v.value("plot_configs",
                                                   nlohmann::json::object())}});
        }
      }
      std::string kind = p.value("kind", std::string("region"));
      if (kind == "variable")
        builder.variable(name, vars, overrides);
      else if (kind == "preset")
        builder.preset(name, vars, overrides);
      else
        builder.region(name, vars, overrides);
    }
  }

  if (cfg.contains("plugins")) {
    for (auto const &p : cfg.at("plugins")) {
      std::string tgt = p.value("target", std::string("analysis"));
      Target target = Target::Analysis;
      if (tgt == "plot")
        target = Target::Plot;
      else if (tgt == "both")
        target = Target::Both;

      std::string id = p.at("id").get<std::string>();
      PluginArgs args;
      if (p.contains("args")) {
        const auto &a = p.at("args");
        if (a.contains("analysis_configs"))
          args.analysis_configs = a.at("analysis_configs");
        if (a.contains("plot_configs"))
          args.plot_configs = a.at("plot_configs");
      }
      builder.add(target, id, args);
    }
  }

  builder.uniqueById();
  PluginSpecList a_specs = builder.analysisSpecs();
  PluginSpecList p_specs = builder.plotSpecs();
  return {a_specs, p_specs};
}

// PipelineRunner orchestrates the execution of the analysis and optional
// plotting stages once a pipeline has been constructed.
class PipelineRunner {
public:
  PipelineRunner(PluginSpecList analysis_specs,
                 PluginSpecList plot_specs)
      : analysis_specs_(std::move(analysis_specs)),
        plot_specs_(std::move(plot_specs)) {}

  // Execute the analysis and plotting for the provided samples
  // configuration. The analysis result is written to \p output_path and
  // returned to the caller.
  inline AnalysisResult run(const nlohmann::json &samples,
                            const std::string &output_path) const {
    auto result = detail::runAnalysis(samples, analysis_specs_);
    result.saveToFile(output_path.c_str());
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
};

} // namespace analysis

