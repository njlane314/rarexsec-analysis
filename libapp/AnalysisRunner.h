#ifndef ANALYSIS_RUNNER_H
#define ANALYSIS_RUNNER_H

#include <algorithm>
#include <map>
#include <memory>
#include <numeric>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include "AnalysisDataLoader.h"
#include "AnalysisDefinition.h"
#include "AnalysisKey.h"
#include "AnalysisPluginManager.h"
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

namespace analysis {

class AnalysisRunner {
    public:
        AnalysisRunner(AnalysisDataLoader &ldr,
                       std::unique_ptr<HistogramFactory> factory,
                       SystematicsProcessor &sys_proc,
                       const nlohmann::json &plgn_cfg)
            : data_loader_(ldr),
              analysis_definition_(selection_registry_),
              systematics_processor_(sys_proc),
              sample_processor_factory_(data_loader_),
              cut_flow_calculator_(data_loader_, analysis_definition_),
              histogram_factory_(std::move(factory)),
              variable_processor_(analysis_definition_, systematics_processor_,
                                  *histogram_factory_) {
            plugin_manager_.loadPlugins(plgn_cfg, &data_loader_);
        }

        AnalysisResult run() {
            log::info("AnalysisRunner::run",
                      "Initiating orchestrated analysis run...");
            plugin_manager_.notifyInitialisation(analysis_definition_,
                                                 selection_registry_);

            analysis_definition_.resolveDynamicBinning(data_loader_);
            RegionAnalysisMap analysis_regions;

            const auto &regions = analysis_definition_.regions();
            size_t region_count = regions.size();
            size_t region_index = 0;
            for (const auto &region_handle : regions) {
                ++region_index;
                log::info("AnalysisRunner::run",
                          "Engaging region protocol (",
                          region_index, "/", region_count,
                          "):", region_handle.key_.str());

                RegionAnalysis region_analysis = std::move(*region_handle.analysis());

                auto [sample_processors, monte_carlo_nodes] =
                    sample_processor_factory_.create(region_handle, region_analysis);

                cut_flow_calculator_.compute(region_handle, region_analysis);
                variable_processor_.process(region_handle, region_analysis,
                                            sample_processors, monte_carlo_nodes);

                analysis_regions[region_handle.key_] = std::move(region_analysis);

                log::info("AnalysisRunner::run",
                          "Region protocol complete (",
                          region_index, "/", region_count,
                          "):", region_handle.key_.str());
            }

            AnalysisResult result(std::move(analysis_regions));
            plugin_manager_.notifyFinalisation(result);
            return result;
        }

    private:
        AnalysisPluginManager plugin_manager_;
        SelectionRegistry selection_registry_;

        AnalysisDataLoader &data_loader_;
        AnalysisDefinition analysis_definition_;
        SystematicsProcessor &systematics_processor_;

        SampleProcessorFactory<AnalysisDataLoader> sample_processor_factory_;
        CutFlowCalculator<AnalysisDataLoader> cut_flow_calculator_;
        std::unique_ptr<HistogramFactory> histogram_factory_;
        VariableProcessor<SystematicsProcessor> variable_processor_;
};

} 

#endif
