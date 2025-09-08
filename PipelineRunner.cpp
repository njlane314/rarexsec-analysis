#include <algorithm>
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
#include "PipelineRunner.h"
#include "PluginAliases.h"
#include "RunConfigLoader.h"
#include "RunConfigRegistry.h"
#include "SystematicsProcessor.h"
#include "VariableRegistry.h"

using analysis::PluginArgs;
using analysis::PluginSpecList;
using analysis::Target;

namespace {

// Process a single beamline given a list of plugin specifications.
analysis::AnalysisResult processBeamline(
    analysis::RunConfigRegistry &run_config_registry,
    const std::string &ntuple_dir, const std::string &beam,
    const nlohmann::json &runs, const PluginSpecList &analysis_specs) {
  std::vector<std::string> periods;
  periods.reserve(runs.size());
  for (auto const &[period, _] : runs.items())
    periods.emplace_back(period);

  analysis::VariableRegistry variable_registry;
  analysis::SystematicsProcessor systematics_processor(variable_registry);
  analysis::AnalysisDataLoader data_loader(run_config_registry,
                                           variable_registry, beam, periods,
                                           ntuple_dir, true);
  auto histogram_factory =
      std::make_unique<analysis::HistogramFactory>();

  analysis::AnalysisRunner runner(data_loader, std::move(histogram_factory),
                                  systematics_processor, analysis_specs);

  return runner.run();
}

void aggregateResults(analysis::AnalysisResult &result,
                      const analysis::AnalysisResult &beamline_result) {
  for (auto &kv : beamline_result.regions())
    result.regions().insert(kv);
}

analysis::AnalysisResult runAnalysis(const nlohmann::json &samples,
                                     const PluginSpecList &analysis_specs) {
  ROOT::EnableImplicitMT();
  analysis::log::info("analysis::runAnalysis",
                      "Implicit multithreading engaged across",
                      ROOT::GetThreadPoolSize(), "threads.");

  std::string ntuple_dir = samples.at("ntupledir").get<std::string>();
  analysis::log::info("analysis::runAnalysis", "Configuration loaded for",
                      samples.at("beamlines").size(), "beamlines.");

  analysis::RunConfigRegistry run_config_registry;
  analysis::RunConfigLoader::loadFromJson(samples, run_config_registry);

  analysis::AnalysisResult result;
  for (auto const &[beam, runs] : samples.at("beamlines").items()) {
    auto beamline_result =
        processBeamline(run_config_registry, ntuple_dir, beam, runs,
                        analysis_specs);
    aggregateResults(result, beamline_result);
  }

  return result;
}

void plotBeamline(analysis::RunConfigRegistry &run_config_registry,
                  const std::string &ntuple_dir, const std::string &beam,
                  const nlohmann::json &runs, const PluginSpecList &plot_specs,
                  const analysis::AnalysisResult &beam_result) {
  std::vector<std::string> periods;
  periods.reserve(runs.size());
  for (auto const &[period, _] : runs.items())
    periods.emplace_back(period);

  analysis::VariableRegistry variable_registry;
  analysis::AnalysisDataLoader data_loader(run_config_registry,
                                           variable_registry, beam, periods,
                                           ntuple_dir, true);

  analysis::PlotPluginHost p_host(&data_loader);
  for (auto const &spec : plot_specs)
    p_host.add(spec.id, spec.args);

  p_host.forEach([&](analysis::IPlotPlugin &pl) { pl.onPlot(beam_result); });
}

void runPlotting(const nlohmann::json &samples,
                 const PluginSpecList &plot_specs,
                 const analysis::AnalysisResult &result) {
  std::string ntuple_dir = samples.at("ntupledir").get<std::string>();
  analysis::log::info("analysis::runPlotting", "Configuration loaded for",
                      samples.at("beamlines").size(), "beamlines.");

  analysis::RunConfigRegistry run_config_registry;
  analysis::RunConfigLoader::loadFromJson(samples, run_config_registry);

  auto result_map = result.resultsByBeam();
  for (auto const &[beam, runs] : samples.at("beamlines").items()) {
    auto it = result_map.find(beam);
    if (it != result_map.end()) {
      plotBeamline(run_config_registry, ntuple_dir, beam, runs, plot_specs,
                   it->second);
    }
  }

  analysis::log::info("analysis::runPlotting",
                      "Plotting routine terminated nominally.");
}

} // namespace

namespace analysis {

std::pair<PluginSpecList, PluginSpecList>
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

PipelineRunner::PipelineRunner(PluginSpecList analysis_specs,
                               PluginSpecList plot_specs)
    : analysis_specs_(std::move(analysis_specs)),
      plot_specs_(std::move(plot_specs)) {}

AnalysisResult PipelineRunner::run(const nlohmann::json &samples,
                                   const std::string &output_path) const {
  auto result = runAnalysis(samples, analysis_specs_);
  result.saveToFile(output_path.c_str());
  runPlotting(samples, plot_specs_, result);
  return result;
}

} // namespace analysis

