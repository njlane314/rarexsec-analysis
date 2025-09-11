#pragma once
#include <algorithm>
#include <map>
#include <memory>
#include <numeric>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "AnalysisDataLoader.h"
#include "AnalysisDefinition.h"
#include "AnalysisKey.h"
#include "AnalysisResult.h"
#include "HistogramFactory.h"
#include "Logger.h"
#include "RegionAnalysis.h"
#include "SampleDataset.h"
#include "SelectionRegistry.h"
#include "SystematicsProcessor.h"
#include "VariableResult.h"

#include "SampleProcessorFactory.h"
#include "CutFlowCalculator.h"
#include "VariableProcessor.h"

#include "PluginAliases.h"
#include "PluginSpec.h"

namespace analysis {

class AnalysisRunner {
public:
  AnalysisRunner(AnalysisDataLoader &ldr,
                 std::unique_ptr<HistogramFactory> factory,
                 SystematicsProcessor &sys_proc,
                 const PluginSpecList &analysis_specs,
                 const PluginSpecList &syst_specs)
    : s_host_(&sys_proc),
      a_host_(&ldr),
      p_host_(&ldr),
      data_loader_(ldr),
      analysis_definition_(selection_registry_),
      systematics_processor_(sys_proc),
      sample_processor_factory_(data_loader_),
      cut_flow_calculator_(data_loader_, analysis_definition_),
      histogram_factory_(std::move(factory)),
      variable_processor_(analysis_definition_, systematics_processor_, *histogram_factory_) {

    for (const auto &s : syst_specs) {
      s_host_.add(s.id, s.args);
    }

    for (const auto &s : analysis_specs) {
      a_host_.add(s.id, s.args);
    }
  }

  AnalysisResult run() {
    log::info("AnalysisRunner::run", "Initiating orchestrated analysis run...");

    // Initialisation callback
    a_host_.forEach([&](IAnalysisPlugin& pl){
      pl.onInitialisation(analysis_definition_, selection_registry_);
    });

    // Configure systematics plugins
    s_host_.forEach([&](ISystematicsPlugin& sp){ sp.configure(systematics_processor_); });

    analysis_definition_.resolveDynamicBinning(data_loader_);
    RegionAnalysisMap analysis_regions;

    const auto &regions = analysis_definition_.regions();
    size_t region_count = regions.size();
    size_t region_index = 0;
    for (const auto &region_handle : regions) {
      ++region_index;
      log::info("AnalysisRunner::run",
                "Engaging region protocol (", region_index, "/", region_count, "):",
                region_handle.key_.str());

      RegionAnalysis region_analysis = std::move(*region_handle.analysis());

      auto [sample_processors, monte_carlo_nodes] =
          sample_processor_factory_.create(region_handle, region_analysis);

      cut_flow_calculator_.compute(region_handle, region_analysis);
      variable_processor_.process(region_handle, region_analysis,
                                  sample_processors, monte_carlo_nodes);

      analysis_regions[region_handle.key_] = std::move(region_analysis);

      log::info("AnalysisRunner::run",
                "Region protocol complete (", region_index, "/", region_count, "):",
                region_handle.key_.str());
    }

    AnalysisResult result(std::move(analysis_regions));

    // Finalisation callback
    a_host_.forEach([&](IAnalysisPlugin& pl){ pl.onFinalisation(result); });

    // Optional: plot host hook if you wire plot specs elsewhere
    // p_host_.forEach([&](IPlotPlugin& pp){ pp.onPlot(result); });

    return result;
  }

private:
  SystematicsPluginHost s_host_;
  AnalysisPluginHost a_host_;
  PlotPluginHost     p_host_; // present for future use

  SelectionRegistry selection_registry_;

  AnalysisDataLoader &data_loader_;
  AnalysisDefinition analysis_definition_;
  SystematicsProcessor &systematics_processor_;

  SampleProcessorFactory<AnalysisDataLoader> sample_processor_factory_;
  CutFlowCalculator<AnalysisDataLoader> cut_flow_calculator_;
  std::unique_ptr<HistogramFactory> histogram_factory_;
  VariableProcessor<SystematicsProcessor> variable_processor_;
};

} // namespace analysis
