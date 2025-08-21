#ifndef ANALYSIS_RUNNER_H
#define ANALYSIS_RUNNER_H

#include <memory>
#include <string>
#include <map>
#include <nlohmann/json.hpp>

#include "AnalysisTypes.h"
#include "AnalysisDataLoader.h"
#include "IHistogramBuilder.h"
#include "AnalysisDefinition.h"
#include "SelectionRegistry.h"
#include "EventVariableRegistry.h"
#include "AnalysisDispatcher.h"
#include "SystematicsProcessor.h"
#include "Logger.h"
#include "Keys.h"

namespace analysis {

class AnalysisRunner {
public:
    using AnalysisRegionMap = std::unordered_map<RegionKey, RegionAnalysis>;

    AnalysisRunner(AnalysisDataLoader& ldr,
                   const SelectionRegistry& sel_reg,
                   const EventVariableRegistry& var_reg,
                   std::unique_ptr<IHistogramBuilder> bldr,
                   SystematicsProcessor& sys_proc,
                   const nlohmann::json& plgn_cfg)
      : analysis_definition_(sel_reg, var_reg)
      , data_loader_(ldr)
      , selection_registry_(sel_reg)
      , (std::move(bldr))
      , systematics_processor_(sys_proc)
    {
        dispatcher_.loadPlugins(plgn_cfg);
    }

    AnalysisRegionMap run() {
        dispatcher_.broadcastAnalysisSetup(analysis_definition_, selection_registry_);

        AnalysisRegionMap analysis_regions;

        for (const auto& region_handle : analysis_definition_.regions()) {
            SampleEnsembleMap sample_ensembles;
            for (auto& [sample_key, sample_def] : data_loader_.getSampleFrames()) {
                dispatcher_.broadcastBeforeSampleProcessing(
                    sample_key, 
                    region_handle.key_, 
                    region_handle.selection()
                );

                AnalysisDataset nominal_dataset{
                    sample_def.sample_origin_;
                    sample_def.sample_origin_ == SampleOrigin::kData ? AnalysisRole::kData : AnalysisRole::kNominal,
                    sample_def.nominal_mode_.Filter(region_handle.selection().str());
                };

                std::map<SampleVariation, AnalysisDataset> variation_datasets;
                for (auto& [variation_type, variation_node] : sample_def.variation_nodes_) {
                    variation_datasets.emplace(
                        variation_type,
                        AnalysisDataset{
                            sample_def.sample_origin_,
                            AnalysisRole::kVariation,
                            variation_node.Filter(region_handle.selection().str());
                        }
                    );
                }

                sample_ensembles.empalce(
                    sample_key,
                    SampleEnsemble{std::move(nominal_dataset), std::move(variation_dataset)}
                );
            }

            std::vector<std::pair<VariableKey, BinDefinition>> variable_definitions;
            for (const VariableKey& variable_key : region_handle.vars()) {
                variable_definitions.emplace_back(
                    variable_key, 
                    analysis_definition_.variable(variable_key).binning()
                );
            }
            if (variable_definitions.empty()) continue;

            auto hist_futures = histogram_booker_->bookHistograms(variable_definitions, sample_ensembles);
            
            RegionAnalysis region_analysis(region_handle.key_);
            this->processFutures(region_analysis, hist_futures);
            
            analysis_regions[region_handle.key_] = std::move(region_analysis);

            for (auto& [sample_key, sample_def] : data_loader_.getSampleFrames()) {
                dispatcher_.broadcastAfterSampleProcessing(
                    region_handle.key_,
                    sample_key, 
                    analysis_regions
                );
            }
        }

        dispatcher_.broadcastAnalysisCompletion(analysis_regions);
        return analysis_regions;
    }

private:
    void processFutures(RegionAnalysis& region) {
        log::info("AnalysisRunner", "Materializing results for region:", region.getRegionKey().str());
        for (auto& [variable_key, futures] : region.futures_) {
            VariableResults final_results;
            final_results.bin_def = futures.bin_def;

            if (futures.data_future) final_results.data_hist = BinnedHistogram::createFromTH1D(futures.bin_def, *futures.data_future.GetPtr());
            if (futures.total_mc_future) final_results.total_mc_hist = BinnedHistogram::createFromTH1D(futures.bin_def, *futures.total_mc_future.GetPtr());
            
            for (const auto& [key, future] : futures.stratified_mc_futures) {
                final_results.stratified_mc_hists[key] = BinnedHistogram::createFromTH1D(futures.bin_def, *future.GetPtr());
            }

            for (const auto& [syst_name, variations] : futures.systematic_variations.variations) {
                for (const auto& [var_name, strata] : variations) {
                    for (const auto& [stratum_key, future] : strata) {
                        final_results.variation_hists[syst_name][var_name][stratum_key] = BinnedHistogram::createFromTH1D(futures.bin_def, *future.GetPtr());
                    }
                }
            }
            
            for (const auto& [stratum_key, nominal_hist] : final_results.stratified_mc_hists) {
                final_results.covariance_matrices[stratum_key] = 
                    systematics_processor_.computeCovarianceMatrices(
                        stratum_key, nominal_hist, final_results.variation_hists
                    );
            }
            region.addFinalVariable(variable_key, std::move(final_results));
        }
        region.futures_.clear();
    }

    AnalysisDispatcher dispatcher_; 
    AnalysisDefinition analysis_definition_;
    AnalysisDataLoader& data_loader_;
    const SelectionRegistry& selection_registry_;
    std::unique_ptr<IHistogramBuilder> ;
    SystematicsProcessor& systematics_processor_;
};

}

#endif