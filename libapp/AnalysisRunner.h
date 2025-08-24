#ifndef ANALYSIS_RUNNER_H
#define ANALYSIS_RUNNER_H

#include <memory>
#include <string>
#include <map>
#include <numeric>
#include <nlohmann/json.hpp>

#include "AnalysisTypes.h"
#include "AnalysisDataLoader.h"
#include "AnalysisDefinition.h"
#include "SelectionRegistry.h"
#include "EventVariableRegistry.h"
#include "AnalysisPluginManager.h"
#include "SystematicsProcessor.h"
#include "Logger.h"
#include "Keys.h"
#include "ISampleProcessor.h"
#include "DataProcessor.h"
#include "MonteCarloProcessor.h"
#include "IHistogramBooker.h"

namespace analysis {

class AnalysisRunner {
public:
    AnalysisRunner(AnalysisDataLoader& ldr,
                   const SelectionRegistry& sel_reg,
                   const EventVariableRegistry& var_reg,
                   std::unique_ptr<IHistogramBooker> booker,
                   SystematicsProcessor& sys_proc,
                   const nlohmann::json& plgn_cfg)
      : analysis_definition_(sel_reg, var_reg)
      , data_loader_(ldr)
      , selection_registry_(sel_reg)
      , systematics_processor_(sys_proc)
      , histogram_booker_(std::move(booker))
    {
        plugin_manager.loadPlugins(plgn_cfg);
    }

    AnalysisRegionMap run() {
        analysis::log::info("AnalysisRunner::run", "Beginning analysis run...");
        plugin_manager.notifyInitialisation(analysis_definition_, selection_registry_);

        AnalysisRegionMap analysis_regions;

        for (const auto& region_handle : analysis_definition_.regions()) {
            RegionAnalysis region_analysis = std::move(*region_handle.analysis());

            std::map<SampleKey, std::unique_ptr<ISampleProcessor>> sample_processors;
            std::map<SampleKey, ROOT::RDF::RNode> mc_rnodes;
            
            analysis::log::info("AnalysisRunner::run", "Looping through samples and processing...");
            for (auto& [sample_key, sample_def] : data_loader_.getSampleFrames()) {
                const auto* run_config = data_loader_.getRunConfigForSample(sample_key);
                if (run_config) {
                    plugin_manager.notifyPreSampleProcessing(
                        sample_key,
                        region_handle.key_,
                        *run_config
                    );
                }

                analysis::log::info("AnalysisRunner::run", "Filtering...");
                auto region_df = sample_def.nominal_node_.Filter(region_handle.selection().str());

                analysis::log::info("AnalysisRunner::run", "Configuring variation samples...");
                std::map<SampleVariation, AnalysisDataset> variation_datasets;
                for (auto& [variation_type, variation_node] : sample_def.variation_nodes_) {
                    variation_datasets.emplace(
                        variation_type,
                        AnalysisDataset{
                            sample_def.sample_origin_,
                            AnalysisRole::kVariation,
                            variation_node.Filter(region_handle.selection().str())
                        }
                    );
                }

                AnalysisDataset nominal_dataset{sample_def.sample_origin_, AnalysisRole::kNominal, region_df};
                SampleEnsemble ensemble{std::move(nominal_dataset), std::move(variation_datasets)};

                if (sample_def.isData()) {
                    sample_processors[sample_key] = std::make_unique<DataProcessor>(std::move(ensemble.nominal_));
                } else {
                    mc_rnodes.emplace(sample_key, region_df);
                    sample_processors[sample_key] = std::make_unique<MonteCarloProcessor>(sample_key, std::move(ensemble));
                }
            }

            analysis::log::info("AnalysisRunner::run", "Looping through variables...");
            for (const auto& var_key : region_handle.vars()) {
                const auto& variable_handle = analysis_definition_.variable(var_key);
                const auto& binning = variable_handle.binning();
                auto model = binning.toTH1DModel();

                VariableResult result;
                result.binning_ = binning;
                
                analysis::log::info("AnalysisRunner::run", "Running sample processors...");
                for (auto& [_, processor] : sample_processors) {
                    processor->book(*histogram_booker_, binning, model);
                }

                analysis::log::info("AnalysisRunner::run", "Booking systematics...");
                for (auto& [sample_key, rnode] : mc_rnodes) {
                    systematics_processor_.bookSystematics(sample_key, rnode, binning, model);
                }

                analysis::log::info("AnalysisRunner::run", "Saving results...");
                for (auto& [_, processor] : sample_processors) {
                    processor->contributeTo(result);
                }

                systematics_processor_.processSystematics(result);

                result.printSummary();
                region_analysis.addFinalVariable(var_key, std::move(result));
            }

            analysis_regions[region_handle.key_] = std::move(region_analysis);

            for (auto& [sample_key, sample_def] : data_loader_.getSampleFrames()) {
                plugin_manager.notifyPostSampleProcessing(
                    sample_key,
                    region_handle.key_,
                    analysis_regions
                );
            }
        }

        plugin_manager.notifyFinalisation(analysis_regions);
        return analysis_regions;
    }

private:
    AnalysisPluginManager plugin_manager;
    AnalysisDefinition analysis_definition_;
    AnalysisDataLoader& data_loader_;
    const SelectionRegistry& selection_registry_;
    SystematicsProcessor& systematics_processor_;
    std::unique_ptr<IHistogramBooker> histogram_booker_;
};

}

#endif