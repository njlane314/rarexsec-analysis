#ifndef ANALYSIS_RUNNER_H
#define ANALYSIS_RUNNER_H

#include <memory>
#include <string>
#include <map>
#include <nlohmann/json.hpp>

#include "AnalysisTypes.h"
#include "AnalysisDataLoader.h"
#include "AnalysisDefinition.h"
#include "SelectionRegistry.h"
#include "EventVariableRegistry.h"
#include "AnalysisPluginManager.h"
#include "SystematicsProcessor.h"
#include "StratifierRegistry.h"
#include "Logger.h"
#include "Keys.h"
#include "ISampleProcessor.h"
#include "DataProcessor.h"
#include "MonteCarloProcessor.h"
#include "HistogramBooker.h"

namespace analysis {

class AnalysisRunner {
public:
    AnalysisRunner(AnalysisDataLoader& ldr,
                   const SelectionRegistry& sel_reg,
                   const EventVariableRegistry& var_reg,
                   SystematicsProcessor& sys_proc,
                   const nlohmann::json& plgn_cfg)
      : analysis_definition_(sel_reg, var_reg)
      , data_loader_(ldr)
      , selection_registry_(sel_reg)
      , systematics_processor_(sys_proc)
      , stratifier_registry_()
      , histogram_booker_(std::make_unique<HistogramBooker>(stratifier_registry_))
    {
        plugin_manager.loadPlugins(plgn_cfg);
    }

    AnalysisRegionMap run() {
        plugin_manager.notifyInitialisation(analysis_definition_, selection_registry_);

        AnalysisRegionMap analysis_regions;

        for (const auto& region_handle : analysis_definition_.regions()) {
            RegionAnalysis region_analysis(region_handle.key_);
            
            std::map<SampleKey, std::unique_ptr<ISampleProcessor>> sample_processors;

            for (auto& [sample_key, sample_def] : data_loader_.getSampleFrames()) {
                plugin_manager.notifyPreSampleProcessing(
                    sample_key,
                    region_handle.key_,
                    region_handle.selection()
                );

                auto region_df = sample_def.nominal_node_.Filter(region_handle.selection().str());

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
                SampleEnsemble ensemble{nominal_dataset, variation_datasets};

                if (sample_def.isData()) {
                    sample_processors[sample_key] = std::make_unique<DataProcessor>(ensemble.nominal_);
                } else {
                    sample_processors[sample_key] = std::make_unique<MonteCarloProcessor>(ensemble);
                }
            }

            for (const auto& var_key : region_handle.vars()) {
                const auto& variable_handle = analysis_definition_.variable(var_key);
                const auto& binning = variable_handle.binning();
                auto model = binning.toTH1DModel();
                
                VariableResult result;
                result.binning_ = binning;

                for (auto& [_, processor] : sample_processors) {
                    processor->book(*histogram_booker_, binning, model);
                }
                
                for (auto& [_, processor] : sample_processors) {
                    processor->contributeTo(result);
                }
                
                for (auto& [stratum_key, nominal_hist] : result.strat_hists_) {
                    result.covariance_matrices_[stratum_key] =
                        systematics_processor_.computeCovarianceMatrices(
                            std::stoi(stratum_key.str()),
                            nominal_hist,
                            result.variation_hists_
                        );
                }
                
                region_analysis.addFinalVariable(var_key, std::move(result));
            }
            
            analysis_regions[region_handle.key_] = std::move(region_analysis);

            for (auto& [sample_key, sample_def] : data_loader_.getSampleFrames()) {
                plugin_manager.notifyPostSampleProcessing(
                    region_handle.key_,
                    sample_key,
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
    StratifierRegistry stratifier_registry_;
    std::unique_ptr<IHistogramBooker> histogram_booker_;
};

}

#endif