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
    using AnalysisResultMap = std::map<std::string, HistogramResult>;

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

    AnalysisResultMap run() {
        log::info("AnalysisRunner", "Setup complete, starting regions loop");
        dispatcher_.broadcastAnalysisSetup(ana_definition_, sel_registry_);

        AnalysisResultMap all_results;

        for (const auto& [region_key, region] : ana_definition_.getRegions()) {
            log::info("AnalysisRunner", "BEGIN REGION:", region_key);
            IHistogramBuilder::SampleDataFrameMap dataframes;

            for (auto& [sample_key, sample] : data_loader_.getSampleFrames()) {
                log::info("AnalysisRunner", "  Pre-sample processing for:", sample_key);
                dispatcher_.broadcastBeforeSampleProcessing(region_key, region, sample_key);
                dataframes.emplace(sample_key, std::make_pair(sample.sample_type_, sample.nominal_node_.Filter(region.filter.str())));
            }

            for (const auto& [var_key, var_cfg] : ana_definition_.getVariables()) {
                log::info("AnalysisRunner", "   Building hist for var:", var_key);
                
                auto hist = hist_builder_->build(var_cfg.bin_def, dataframes);
                hist.pot = data_loader_.getTotalPot();
                hist.beam = data_loader_.getBeam();
                hist.runs = data_loader_.getPeriods();
                hist.region = region.region_name;
                hist.axis_label = var_cfg.axis_label;
                
                std::string result_key = var_key + "@" + region_key;
                log::info("AnalysisRunner", "Storing final histogram with key:", result_key);

                all_results[result_key] = hist;
            }
            
            for (auto& [sample_key, sample] : data_loader_.getSampleFrames()){
                log::info("AnalysisRunner", "  Post-sample processing for:", sample_key);
                dispatcher_.broadcastAfterSampleProcessing(region_key, sample_key, all_results);
            }
            log::info("AnalysisRunner", "END   REGION:", region_key);
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

#endif