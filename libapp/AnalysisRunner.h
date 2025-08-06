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
#include "Logger.h"

namespace analysis {

class AnalysisRunner {
public:
    AnalysisRunner(AnalysisDataLoader& ldr,
                   const SelectionRegistry& sreg,
                   const EventVariableRegistry& var_reg,
                   std::unique_ptr<IHistogramBuilder> bldr,
                   const nlohmann::json& plugin_config)
      : ana_definition_(sreg, var_reg)
      , data_loader_(ldr)
      , sel_registry_(sreg)
      , hist_builder_(std::move(bldr))
    {
        dispatcher_.loadPlugins(plugin_config);
    }

    HistogramResult run() {
        dispatcher_.broadcastAnalysisSetup(ana_definition_, sel_registry_);

        HistogramResult all_results;

        for (const auto& [region_key, region] : ana_definition_.getRegions()) {
            auto query = sel_registry_.getRegionFilterQuery(region);
            std::map<std::string, ROOT::RDF::RNode> dataframes;

            for (auto& [sample_key, sample] : data_loader_.getSampleFrames()) {
                dispatcher_.broadcastBeforeSampleProcessing(region_key, region, sample_key);
                dataframes.emplace(sample_key, std::make_pair(sample.sample_type_, sample.nominal_node_.Filter(query.str())));
            }

            for (const auto& [var_key, var_cfg] : ana_definition_.getVariables()) {
                auto hist = hist_builder_->build(var_cfg.bin_def, dataframes);
                std::string result_key = var_key + "@" + region_key;
                log::info("AnalysisRunner", "Storing final histogram with key:", result_key);
                all_results.channels[result_key] = hist.total;
            }
            
            for (auto& [sample_key, sample] : data_loader_.getSampleFrames()){
                 dispatcher_.broadcastAfterSampleProcessing(region_key, sample_key, all_results);
            }
        }

        dispatcher_.broadcastAnalysisCompletion(all_results);
        return all_results;
    }

private:
    AnalysisDefinition ana_definition_;
    AnalysisCallbackDispatcher dispatcher_;

    AnalysisDataLoader& data_loader_;
    const SelectionRegistry& sel_registry_;
    std::unique_ptr<IHistogramBuilder> hist_builder_;
};

}

#endif // ANALYSIS_RUNNER_H