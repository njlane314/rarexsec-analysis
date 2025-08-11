#ifndef ANALYSIS_RUNNER_H
#define ANALYSIS_RUNNER_H

#include <memory>
#include <string>
#include <map>
#include <nlohmann/json.hpp>

#include "AnalysisDataLoader.h"
#include "IHistogramBuilder.h"
#include "HistogramResult.h"
#include "AnalysisDefinition.h"
#include "SelectionRegistry.h"
#include "EventVariableRegistry.h"
#include "AnalysisCallbackDispatcher.h"
#include "SystematicsProcessor.h"
#include "Logger.h"

namespace analysis {

class AnalysisRunner {
public:
    using AnalysisRegionMap = std::map<RegionKey, RegionAnalysis>;

    AnalysisRunner(AnalysisDataLoader& ldr,
                   const SelectionRegistry& sel_reg,
                   const EventVariableRegistry& var_reg,
                   std::unique_ptr<IHistogramBuilder> bldr,
                   SystematicsProcessor& sys_proc,
                   const nlohmann::json& plgn_cfg)
      : analysis_definition_(sel_reg, var_reg)
      , data_loader_(ldr)
      , selection_registry_(sel_reg)
      , histogram_builder_(std::move(bldr))
      , systematics_processor_(sys_proc)
    {
        dispatcher_.loadPlugins(plgn_cfg);
    }

    AnalysisRegionMap run() {
        dispatcher_.broadcastAnalysisSetup(analysis_definition_, selection_registry_);

        AnalysisRegionMap analysis_regions;
        for (const auto& region_view : analysis_definition_.regions()) {
            IHistogramBuilder::SampleDataFrameMap sample_dataframes;
            for (auto& [sample_key, sample] : data_loader_.samples()) {
                dispatcher_.broadcastBeforeSampleProcessing(
                    region_view.key.str(), 
                    region_view.name(),
                    region_view.selection(),
                    sample_key
                );

                sample_dataframes.emplace(sample_key, std::make_pair(
                    sample.type, 
                    sample.nominal.Filter(region_view.selection().str())
                ));
            }

            std::vector<std::pair<VariableKey, BinDefinition>> variable_definitions;
            const auto& region_variables = region_view.variables();
            
            if (region_variables.empty()) 
                continue;
            
            for (const VariableKey& variable_key : region_variables) {
                auto variable_view = analysis_definition_.variable(variable_key.str());
                variable_definitions.emplace_back(variable_key, variable_view.bins());
            }

            auto built_region = histogram_builder_->buildRegionAnalysis(
                region_view.key, variable_definitions, sample_dataframes
            );

            analysis_regions[region_view.key] = std::move(built_region);
            
            for (auto& [sample_key, sample] : data_loader_.samples()) {
                dispatcher_.broadcastAfterSampleProcessing(
                    region_view.key.str(), sample_key, analysis_regions
                );
            }
        }

        dispatcher_.broadcastAnalysisCompletion(analysis_regions);
        return analysis_regions;
    }


private:
    AnalysisCallbackDispatcher dispatcher_; 

    AnalysisDefinition analysis_definition_;
    AnalysisDataLoader& data_loader_;
    
    const SelectionRegistry& selection_registry_;
    std::unique_ptr<IHistogramBuilder> histogram_builder_;
    SystematicsProcessor& systematics_processor_;
};

}

#endif