#ifndef ANALYSIS_RUNNER_H
#define ANALYSIS_RUNNER_H

#include <algorithm>
#include <map>
#include <memory>
#include <numeric>
#include <set>
#include <string>
#include <unordered_map>

#include <nlohmann/json.hpp>

#include "AnalysisDataLoader.h"
#include "AnalysisDefinition.h"
#include "AnalysisLogger.h"
#include "AnalysisPluginManager.h"
#include "AnalysisResult.h"
#include "AnalysisTypes.h"
#include "DataProcessor.h"
#include "EventVariableRegistry.h"
#include "IHistogramBooker.h"
#include "ISampleProcessor.h"
#include "KeyTypes.h"
#include "MonteCarloProcessor.h"
#include "SelectionRegistry.h"
#include "SystematicsProcessor.h"
#include <tbb/parallel_for_each.h>

namespace analysis {

class AnalysisRunner {
  public:
    AnalysisRunner(AnalysisDataLoader &ldr, const SelectionRegistry &sel_reg, const EventVariableRegistry &var_reg,
                   std::unique_ptr<IHistogramBooker> booker, SystematicsProcessor &sys_proc,
                   const nlohmann::json &plgn_cfg)
        : analysis_definition_(sel_reg, var_reg), data_loader_(ldr), selection_registry_(sel_reg),
          systematics_processor_(sys_proc), histogram_booker_(std::move(booker)) {
        plugin_manager.loadPlugins(plgn_cfg, &data_loader_);
    }

    AnalysisResult run() {
        log::info("AnalysisRunner::run", "Initiating orchestrated analysis run...");
        plugin_manager.notifyInitialisation(analysis_definition_, selection_registry_);

        analysis_definition_.resolveDynamicBinning(data_loader_);

        RegionAnalysisMap analysis_regions;

        const auto &regions = analysis_definition_.regions();
        size_t region_count = regions.size();
        size_t region_index = 0;
        for (const auto &region_handle : regions) {
            ++region_index;
            log::info("AnalysisRunner::run", "Engaging region protocol (", region_index, "/", region_count,
                      "):", region_handle.key_.str());

            RegionAnalysis region_analysis = std::move(*region_handle.analysis());

            std::unordered_map<SampleKey, std::unique_ptr<ISampleProcessor>> sample_processors;
            std::unordered_map<SampleKey, ROOT::RDF::RNode> monte_carlo_nodes;

            log::info("AnalysisRunner::run", "Processing sample ensemble...");
            auto &sample_frames = data_loader_.getSampleFrames();

            const std::string region_beam = region_analysis.beamConfig();
            const auto &region_runs = region_analysis.runNumbers();

            size_t sample_total = 0;
            for (auto &[sample_key, _] : sample_frames) {
                const auto *run_config = data_loader_.getRunConfigForSample(sample_key);
                if (!run_config)
                    continue;
                if (!region_beam.empty() && run_config->beam_mode != region_beam)
                    continue;
                if (!region_runs.empty() &&
                    std::find(region_runs.begin(), region_runs.end(), run_config->run_period) == region_runs.end())
                    continue;
                ++sample_total;
            }

            size_t sample_index = 0;
            std::set<std::string> accounted_runs;
            for (auto &[sample_key, sample_def] : sample_frames) {
                const auto *run_config = data_loader_.getRunConfigForSample(sample_key);
                if (!run_config)
                    continue;
                if (!region_beam.empty() && run_config->beam_mode != region_beam)
                    continue;
                if (!region_runs.empty() &&
                    std::find(region_runs.begin(), region_runs.end(), run_config->run_period) == region_runs.end())
                    continue;
                ++sample_index;

                if (accounted_runs.insert(run_config->label()).second) {
                    region_analysis.addProtonsOnTarget(run_config->nominal_pot);
                }
                plugin_manager.notifyPreSampleProcessing(sample_key, region_handle.key_, *run_config);

                log::info("AnalysisRunner::run", "--> Conditioning sample (", sample_index, "/", sample_total,
                          "):", sample_key.str());

                auto region_df = sample_def.nominal_node_;
                auto selection_expr = region_handle.selection().str();
                if (!selection_expr.empty()) {
                    region_df = region_df.Filter(selection_expr);
                }

                log::info("AnalysisRunner::run", "Configuring systematic variations...");
                std::unordered_map<SampleVariation, SampleDataset> variation_datasets;
                for (auto &[variation_type, variation_node] : sample_def.variation_nodes_) {
                    auto variation_df = variation_node;
                    if (!selection_expr.empty()) {
                        variation_df = variation_df.Filter(selection_expr);
                    }
                    variation_datasets.emplace(variation_type, SampleDataset{sample_def.sample_origin_,
                                                                             AnalysisRole::kVariation, variation_df});
                }

                SampleDataset nominal_dataset{sample_def.sample_origin_, AnalysisRole::kNominal, region_df};
                SampleDatasetGroup ensemble{std::move(nominal_dataset), std::move(variation_datasets)};

                if (sample_def.isData()) {
                    sample_processors[sample_key] = std::make_unique<DataProcessor>(std::move(ensemble.nominal_));
                } else {
                    monte_carlo_nodes.emplace(sample_key, region_df);
                    sample_processors[sample_key] =
                        std::make_unique<MonteCarloProcessor>(sample_key, std::move(ensemble));
                }
            }

            log::info("AnalysisRunner::run", "Sample processing sequence complete.");

            log::info("AnalysisRunner::run", "Iterating across observable variables...");
            const auto &vars = region_handle.vars();
            size_t var_total = vars.size();
            size_t var_index = 0;
            for (const auto &var_key : vars) {
                ++var_index;
                const auto &variable_handle = analysis_definition_.variable(var_key);
                const auto &binning = variable_handle.binning();
                auto model = binning.toTH1DModel();

                VariableResult result;
                result.binning_ = binning;

                log::info("AnalysisRunner::run", "Deploying variable pipeline (", var_index, "/", var_total,
                          "):", var_key.str());
                log::info("AnalysisRunner::run", "Executing sample processors...");
                tbb::parallel_for_each(sample_processors.begin(), sample_processors.end(),
                                       [&](auto &p) { p.second->book(*histogram_booker_, binning, model); });

                log::info("AnalysisRunner::run", "Registering systematic variations...");
                tbb::parallel_for_each(monte_carlo_nodes.begin(), monte_carlo_nodes.end(), [&](auto &p) {
                    systematics_processor_.bookSystematics(p.first, p.second, binning, model);
                });

                log::info("AnalysisRunner::run", "Persisting results...");
                tbb::parallel_for_each(sample_processors.begin(), sample_processors.end(),
                                       [&](auto &p) { p.second->contributeTo(result); });

                log::info("AnalysisRunner::run", "Computing systematic covariances");
                systematics_processor_.processSystematics(result);
                systematics_processor_.clearFutures();

                result.printSummary();

                region_analysis.addFinalVariable(var_key, std::move(result));

                log::info("AnalysisRunner::run", "Variable pipeline concluded (", var_index, "/", var_total,
                          "):", var_key.str());
            }

            analysis_regions[region_handle.key_] = std::move(region_analysis);

            for (auto &[sample_key, _] : sample_processors) {
                plugin_manager.notifyPostSampleProcessing(sample_key, region_handle.key_, analysis_regions);
            }

            log::info("AnalysisRunner::run", "Region protocol complete (", region_index, "/", region_count,
                      "):", region_handle.key_.str());
        }

        AnalysisResult result(std::move(analysis_regions));
        plugin_manager.notifyFinalisation(result);
        return result;
    }

  private:
    AnalysisPluginManager plugin_manager;
    AnalysisDefinition analysis_definition_;
    AnalysisDataLoader &data_loader_;
    const SelectionRegistry &selection_registry_;
    SystematicsProcessor &systematics_processor_;
    std::unique_ptr<IHistogramBooker> histogram_booker_;
};

} // namespace analysis

#endif
