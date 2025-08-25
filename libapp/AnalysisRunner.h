#ifndef ANALYSIS_RUNNER_H
#define ANALYSIS_RUNNER_H

#include <algorithm>
#include <map>
#include <memory>
#include <numeric>
#include <set>
#include <string>

#include <nlohmann/json.hpp>

#include "DynamicBinning.h"
#include "AnalysisDataLoader.h"
#include "AnalysisDefinition.h"
#include "AnalysisLogger.h"
#include "AnalysisPluginManager.h"
#include "AnalysisTypes.h"
#include "DataProcessor.h"
#include "EventVariableRegistry.h"
#include "IHistogramBooker.h"
#include "ISampleProcessor.h"
#include "KeyTypes.h"
#include "MonteCarloProcessor.h"
#include "SelectionRegistry.h"
#include "SystematicsProcessor.h"

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

    RegionAnalysisMap run() {
        analysis::log::info("AnalysisRunner::run", "Initiating orchestrated analysis run...");
        plugin_manager.notifyInitialisation(analysis_definition_, selection_registry_);

        for (const auto& var_handle : analysis_definition_.variables()) {
            if (analysis_definition_.isDynamic(var_handle.key_)) {
                log::info("AnalysisRunner::run", "Deriving dynamic bin schema for variable:", var_handle.key_.str());

                std::vector<ROOT::RDF::RNode> mc_nodes;
                for (auto& [sample_key, sample_def] : data_loader_.getSampleFrames()) {
                    if (sample_def.isMc()) {
                        mc_nodes.emplace_back(sample_def.nominal_node_);
                    }
                }

                if (mc_nodes.empty()) {
                    log::fatal("AnalysisRunner::run", "Cannot perform dynamic binning: No Monte Carlo samples were found!");
                }

                // Aim: per-bin statistical error ≤ 5% so it's sub-dominant to ~10% systematics.
                // For weighted counts: n_eff = (sum w_i)^2 / (sum w_i^2), and rel_stat_err ≈ 1/sqrt(n_eff).
                // Hence min_neff_per_bin = 400 (~5% stats; total ≈ sqrt(0.10^2 + 0.05^2) ≈ 11.2%).
                bool include_oob = analysis_definition_.includeOutOfRangeBins(var_handle.key_);
                BinningDefinition new_bins = DynamicBinning::calculate(
                    mc_nodes,
                    var_handle.binning(),
                    "nominal_event_weight",
                    400.0,
                    0.0,
                    1.0,
                    include_oob
                );

                log::info("AnalysisRunner::run", "--> Optimal bin count resolved:", new_bins.getBinNumber());

                analysis_definition_.setBinning(var_handle.key_, std::move(new_bins));
            }
        }

        RegionAnalysisMap analysis_regions;

        const auto& regions = analysis_definition_.regions();
        size_t region_count = regions.size();
        size_t region_index = 0;
        for (const auto& region_handle : regions) {
            ++region_index;
            analysis::log::info("AnalysisRunner::run", "Engaging region protocol (", region_index, "/", region_count, "):", region_handle.key_.str());

            RegionAnalysis region_analysis = std::move(*region_handle.analysis());

                std::map<SampleKey, std::unique_ptr<ISampleProcessor>> sample_processors;
                std::map<SampleKey, ROOT::RDF::RNode> mc_rnodes;

            analysis::log::info("AnalysisRunner::run", "Processing sample ensemble...");
            auto& sample_frames = data_loader_.getSampleFrames();

            const std::string region_beam = region_analysis.beamConfig();
            const auto& region_runs = region_analysis.runNumbers();

            size_t sample_total = 0;
            for (auto& [sample_key, _] : sample_frames) {
                const auto* run_config = data_loader_.getRunConfigForSample(sample_key);
                if (!run_config) continue;
                if (!region_beam.empty() && run_config->beam_mode != region_beam) continue;
                if (!region_runs.empty() &&
                    std::find(region_runs.begin(), region_runs.end(), run_config->run_period) == region_runs.end())
                    continue;
                ++sample_total;
            }

            size_t sample_index = 0;
            std::set<std::string> accounted_runs;
            for (auto& [sample_key, sample_def] : sample_frames) {
                const auto* run_config = data_loader_.getRunConfigForSample(sample_key);
                if (!run_config) continue;
                if (!region_beam.empty() && run_config->beam_mode != region_beam) continue;
                if (!region_runs.empty() &&
                    std::find(region_runs.begin(), region_runs.end(), run_config->run_period) == region_runs.end())
                    continue;
                ++sample_index;

                if (accounted_runs.insert(run_config->label()).second) {
                    region_analysis.setProtonsOnTarget(
                        region_analysis.protonsOnTarget() + run_config->nominal_pot);
                }
                plugin_manager.notifyPreSampleProcessing(
                    sample_key,
                    region_handle.key_,
                    *run_config
                );

                analysis::log::info(
                    "AnalysisRunner::run",
                    "--> Conditioning sample (",
                    sample_index,
                    "/",
                    sample_total,
                    "):",
                    sample_key.str()
                );

                auto region_df = sample_def.nominal_node_.Filter(region_handle.selection().str());

                analysis::log::info("AnalysisRunner::run", "Configuring systematic variations...");
                std::map<SampleVariation, SampleDataset> variation_datasets;
                for (auto& [variation_type, variation_node] : sample_def.variation_nodes_) {
                    variation_datasets.emplace(
                        variation_type,
                        SampleDataset{
                            sample_def.sample_origin_,
                            AnalysisRole::kVariation,
                            variation_node.Filter(region_handle.selection().str())
                        }
                    );
                }

                SampleDataset nominal_dataset{sample_def.sample_origin_, AnalysisRole::kNominal, region_df};
                SampleDatasetGroup ensemble{std::move(nominal_dataset), std::move(variation_datasets)};

                if (sample_def.isData()) {
                    sample_processors[sample_key] = std::make_unique<DataProcessor>(std::move(ensemble.nominal_));
                } else {
                    mc_rnodes.emplace(sample_key, region_df);
                    sample_processors[sample_key] = std::make_unique<MonteCarloProcessor>(sample_key, std::move(ensemble));
                }
            }

            analysis::log::info("AnalysisRunner::run", "Sample processing sequence complete.");

            analysis::log::info("AnalysisRunner::run", "Iterating across observable variables...");
            const auto& vars = region_handle.vars();
            size_t var_total = vars.size();
            size_t var_index = 0;
            for (const auto& var_key : vars) {
                ++var_index;
                const auto& variable_handle = analysis_definition_.variable(var_key);
                const auto& binning = variable_handle.binning();
                auto model = binning.toTH1DModel();

                VariableResult result;
                result.binning_ = binning;

                analysis::log::info("AnalysisRunner::run", "Deploying variable pipeline (", var_index, "/", var_total, "):", var_key.str());
                analysis::log::info("AnalysisRunner::run", "Executing sample processors...");
                for (auto& [_, processor] : sample_processors) {
                    processor->book(*histogram_booker_, binning, model);
                }

                analysis::log::info("AnalysisRunner::run", "Registering systematic variations...");
                for (auto& [sample_key, rnode] : mc_rnodes) {
                    systematics_processor_.bookSystematics(sample_key, rnode, binning, model);
                }

                analysis::log::info("AnalysisRunner::run", "Persisting results...");
                for (auto& [_, processor] : sample_processors) {
                    processor->contributeTo(result);
                }
                
                //systematics_processor_.processSystematics(result);

                result.printSummary();
                region_analysis.addFinalVariable(var_key, std::move(result));

                analysis::log::info("AnalysisRunner::run", "Variable pipeline concluded (", var_index, "/", var_total, "):", var_key.str());
            }

            analysis_regions[region_handle.key_] = std::move(region_analysis);

            for (auto& [sample_key, _] : sample_processors) {
                plugin_manager.notifyPostSampleProcessing(
                    sample_key,
                    region_handle.key_,
                    analysis_regions
                );
            }

            analysis::log::info("AnalysisRunner::run", "Region protocol complete (", region_index, "/", region_count, "):", region_handle.key_.str());
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


