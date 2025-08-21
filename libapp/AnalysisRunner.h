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
#include "AnalysisPluginManager.h"
#include "SystematicsProcessor.h"
#include "StratifierRegistry.h" 
#include "Logger.h"
#include "Keys.h"

namespace analysis {

class AnalysisRunner {
public:
    AnalysisRunner(AnalysisDataLoader& ldr,
                   const SelectionRegistry& sel_reg,
                   const EventVariableRegistry& var_reg,
                   std::unique_ptr<IHistogramBuilder> bldr,
                   SystematicsProcessor& sys_proc,
                   const nlohmann::json& plgn_cfg)
      : analysis_definition_(sel_reg, var_reg)
      , data_loader_(ldr)
      , selection_registry_(sel_reg)
      , histogram_booker_(std::move(bldr))
      , systematics_processor_(sys_proc)
    {
        plugin_manager.loadPlugins(plgn_cfg);
    }

    AnalysisRegionMap run() {
        plugin_manager.notifyInitialisation(analysis_definition_, selection_registry_);

        AnalysisRegionMap analysis_regions;

        for (const auto& region_handle : analysis_definition_.regions()) {
            SampleEnsembleMap sample_ensembles;
            for (auto& [sample_key, sample_def] : data_loader_.getSampleFrames()) {
                plugin_manager.notifyPreSampleProcessing(
                    sample_key, 
                    region_handle.key_, 
                    region_handle.selection()
                );

                AnalysisDataset nominal_dataset{
                    sample_def.sample_origin_,
                    sample_def.sample_origin_ == SampleOrigin::kData ? AnalysisRole::kData : AnalysisRole::kNominal,
                    sample_def.nominal_node_.Filter(region_handle.selection().str())
                };

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

                sample_ensembles.emplace(
                    sample_key,
                    SampleEnsemble{std::move(nominal_dataset), std::move(variation_datasets)}
                );
            }

            std::vector<std::pair<VariableKey, BinDefinition>> variable_definitions;
            for (const VariableKey& var_key : region_handle.vars()) {
                variable_definitions.emplace_back(
                    var_key, 
                    analysis_definition_.variable(var_key).binning()
                );
            }
            if (variable_definitions.empty()) continue;

            auto hist_futures = histogram_booker_->bookHistograms(variable_definitions, sample_ensembles);
            
            RegionAnalysis region_analysis(region_handle.key_);
            this->materialiseResults(region_analysis, hist_futures);
            
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
    void materialiseResults(RegionAnalysis& region, std::map<VariableKey, VariableFutures>& futures_map) {
        log::info("AnalysisRunner", "Materializing results for analysis region:", region.getRegionKey().str());

        auto materialise = [](const auto& hist_futur, const auto& binning, const StratumProperties& style) {
            return BinnedHistogram::createFromTH1D(binning, *hist_futur.GetPtr(), style.plain_name.c_str(), style.tex_label.c_str(), style.fill_colour, style.fill_style);
        };
    
        for (auto& [var_key, var_future] : futures_map) {
            VariableResults var_results;
            var_results.binning_ = var_future.binning_;
            const std::string& scheme_name = var_future.binning_.getStratifierKey().str();

            if (var_future.data_hist_) {
                int data_key = stratifier_registry_.findStratumKeyByName(scheme_name, "Data");
                const StratumProperties& props = stratifier_registry_.getStratumProperties(scheme_name, data_key);
                var_results.data_hist_ = materialise(var_future.data_hist_, var_future.binning_, props);
            }

            if (var_future.total_hist_) {
                var_results.total_hist_ = BinnedHistogram::createFromTH1D(var_future.binning_, *var_future.total_hist_.GetPtr());
            }

            for (const auto& [strat_key, hist_futur] : var_future.strat_hists_) {
                const StratumProperties& props = stratifier_registry_.getStratumProperties(scheme_name, strat_key);
                var_results.strat_hists_[strat_key] = materialise(hist_futur, var_future.binning_, props);
            }

            for (const auto& [var_key, hist_futur] : var_future.variation_hists_) {
                var_results.variation_hists_

                for (const auto& [var_name, strata] : variations) {
                    for (const auto& [stratum_key, hist_futur] : strata) {
                        var_results.variation_hists_[var_key][var_name][stratum_key] = BinnedHistogram::createFromTH1D(var_future.binning_, *hist_futur.GetPtr());
                    }
                }
            }
    
            for (const auto& [stratum_key, nominal_hist] : var_results.strat_hists_) {
                var_results.covariance_matrices_[stratum_key] =
                    systematics_processor_.computeCovarianceMatrices(
                        stratum_key, nominal_hist, var_results.variation_hists_
                    );
            }
            region.addFinalVariable(var_key, std::move(var_results));
        }
        futures_map.clear();
    }

    AnalysisPluginManager plugin_manager; 
    AnalysisDefinition analysis_definition_;
    AnalysisDataLoader& data_loader_;
    const SelectionRegistry& selection_registry_;
    std::unique_ptr<IHistogramBuilder> histogram_booker_;
    SystematicsProcessor& systematics_processor_;
    StratifierRegistry stratifier_registry_;
};

}

#endif