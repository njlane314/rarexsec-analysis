#ifndef ANALYSIS_RUNNER_H
#define ANALYSIS_RUNNER_H

#include <algorithm>
#include <map>
#include <memory>
#include <numeric>
#include <set>
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
#include "DataProcessor.h"
#include "HistogramFactory.h"
#include "ISampleProcessor.h"
#include "Logger.h"
#include "MonteCarloProcessor.h"
#include "RegionAnalysis.h"
#include "SampleDataset.h"
#include "SelectionRegistry.h"
#include "StratifierRegistry.h"
#include "SystematicsProcessor.h"
#include "VariableResult.h"
#include <tbb/parallel_for_each.h>

namespace analysis {

class AnalysisRunner {
public:
  AnalysisRunner(AnalysisDataLoader &ldr,
                 std::unique_ptr<HistogramFactory> factory,
                 SystematicsProcessor &sys_proc, const nlohmann::json &plgn_cfg)
      : data_loader_(ldr), analysis_definition_(selection_registry_),
        systematics_processor_(sys_proc),
        histogram_factory_(std::move(factory)) {
    plugin_manager_.loadPlugins(plgn_cfg, &data_loader_);
  }

  AnalysisResult run() {
    log::info("AnalysisRunner::run", "Initiating orchestrated analysis run...");
    plugin_manager_.notifyInitialisation(analysis_definition_,
                                         selection_registry_);

    analysis_definition_.resolveDynamicBinning(data_loader_);
    RegionAnalysisMap analysis_regions;

    const auto &regions = analysis_definition_.regions();
    size_t region_count = regions.size();
    size_t region_index = 0;
    for (const auto &region_handle : regions) {
      ++region_index;
      log::info("AnalysisRunner::run", "Engaging region protocol (",
                region_index, "/", region_count,
                "):", region_handle.key_.str());

      RegionAnalysis region_analysis = std::move(*region_handle.analysis());

      auto [sample_processors, monte_carlo_nodes] =
          this->prepareSampleProcessors(region_handle, region_analysis);

      this->computeCutFlow(region_handle, region_analysis);
      this->processVariables(region_handle, region_analysis, sample_processors,
                             monte_carlo_nodes);

      analysis_regions[region_handle.key_] = std::move(region_analysis);

      log::info("AnalysisRunner::run", "Region protocol complete (",
                region_index, "/", region_count,
                "):", region_handle.key_.str());
    }

    AnalysisResult run() {
        log::info("AnalysisRunner::run", "Initiating orchestrated analysis run...");
        plugin_manager_.notifyInitialisation(analysis_definition_, selection_registry_);

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

            auto [sample_processors, monte_carlo_nodes] =
                this->prepareSampleProcessors(region_handle, region_analysis);

            this->computeCutFlow(region_handle, region_analysis);
            this->processVariables(region_handle, region_analysis, sample_processors, monte_carlo_nodes);

            analysis_regions[region_handle.key_] = std::move(region_analysis);

            log::info("AnalysisRunner::run", "Region protocol complete (", region_index, "/", region_count,
                      "):", region_handle.key_.str());
        }

        AnalysisResult result(std::move(analysis_regions));
        plugin_manager_.notifyFinalisation(result);
        return result;
    }

  private:
    bool isSampleEligible(const SampleKey &sample_key, const RunConfig *run_config,
                          const std::string &region_beam,
                          const std::vector<std::string> &region_runs) const {
        static_cast<void>(sample_key);

        if (!run_config)
            return false;
        if (!region_beam.empty() && run_config->beam_mode != region_beam)
            return false;
        if (!region_runs.empty() &&
            std::find(region_runs.begin(), region_runs.end(), run_config->run_period) == region_runs.end())
            return false;

        return true;
    }

    auto prepareSampleProcessors(const RegionHandle &region_handle, RegionAnalysis &region_analysis)
        -> std::pair<std::unordered_map<SampleKey, std::unique_ptr<ISampleProcessor>>,
                     std::unordered_map<SampleKey, ROOT::RDF::RNode>> {
        std::unordered_map<SampleKey, std::unique_ptr<ISampleProcessor>> sample_processors;
        std::unordered_map<SampleKey, ROOT::RDF::RNode> monte_carlo_nodes;

        log::info("AnalysisRunner::run", "Processing sample ensemble...");
        auto &sample_frames = data_loader_.getSampleFrames();

        const std::string region_beam = region_analysis.beamConfig();
        const auto &region_runs = region_analysis.runNumbers();

        size_t sample_total = 0;
        for (auto &[sample_key, _] : sample_frames) {
            const auto *run_config = data_loader_.getRunConfigForSample(sample_key);
            if (!this->isSampleEligible(sample_key, run_config, region_beam, region_runs))
                continue;
            ++sample_total;
        }

        size_t sample_index = 0;
        std::set<std::string> accounted_runs;
        for (auto &[sample_key, sample_def] : sample_frames) {
            const auto *run_config = data_loader_.getRunConfigForSample(sample_key);
            if (!this->isSampleEligible(sample_key, run_config, region_beam, region_runs))
                continue;
            ++sample_index;

            if (accounted_runs.insert(run_config->label()).second) {
                region_analysis.addProtonsOnTarget(run_config->nominal_pot);
            }
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
                variation_datasets.emplace(
                    variation_type, SampleDataset{sample_def.sample_origin_, AnalysisRole::kVariation, variation_df});
            }

            SampleDataset nominal_dataset{sample_def.sample_origin_, AnalysisRole::kNominal, region_df};
            SampleDatasetGroup ensemble{std::move(nominal_dataset), std::move(variation_datasets)};

            if (sample_def.isData()) {
                sample_processors[sample_key] = std::make_unique<DataProcessor>(std::move(ensemble.nominal_));
            } else {
                monte_carlo_nodes.emplace(sample_key, region_df);
                sample_processors[sample_key] = std::make_unique<MonteCarloProcessor>(sample_key, std::move(ensemble));
            }
    AnalysisResult result(std::move(analysis_regions));
    plugin_manager_.notifyFinalisation(result);
    return result;
  }

private:
  auto prepareSampleProcessors(const RegionHandle &region_handle,
                               RegionAnalysis &region_analysis)
      -> std::pair<
          std::unordered_map<SampleKey, std::unique_ptr<ISampleProcessor>>,
          std::unordered_map<SampleKey, ROOT::RDF::RNode>> {
    std::unordered_map<SampleKey, std::unique_ptr<ISampleProcessor>>
        sample_processors;
    std::unordered_map<SampleKey, ROOT::RDF::RNode> monte_carlo_nodes;

    log::info("AnalysisRunner::run", "Processing sample ensemble...");
    auto &sample_frames = data_loader_.getSampleFrames();

    const std::string region_beam = region_analysis.beamConfig();
    const auto &region_runs = region_analysis.runNumbers();

    size_t sample_total = 0;
    for (auto &[sample_key, _] : sample_frames) {
      const auto *run_config = data_loader_.getRunConfigForSample(sample_key);
      if (!this->isSampleEligible(sample_key, run_config, region_beam,
                                  region_runs))
        continue;
      ++sample_total;
    }

    size_t sample_index = 0;
    std::set<std::string> accounted_runs;
    for (auto &[sample_key, sample_def] : sample_frames) {
      const auto *run_config = data_loader_.getRunConfigForSample(sample_key);
      if (!this->isSampleEligible(sample_key, run_config, region_beam,
                                  region_runs))
        continue;
      ++sample_index;

      if (accounted_runs.insert(run_config->label()).second) {
        region_analysis.addProtonsOnTarget(run_config->nominal_pot);
      }
      log::info("AnalysisRunner::run", "--> Conditioning sample (",
                sample_index, "/", sample_total, "):", sample_key.str());

      auto region_df = sample_def.nominal_node_;
      auto selection_expr = region_handle.selection().str();
      if (!selection_expr.empty()) {
        region_df = region_df.Filter(selection_expr);
      }

      log::info("AnalysisRunner::run", "Configuring systematic variations...");
      std::unordered_map<SampleVariation, SampleDataset> variation_datasets;
      for (auto &[variation_type, variation_node] :
           sample_def.variation_nodes_) {
        auto variation_df = variation_node;
        if (!selection_expr.empty()) {
          variation_df = variation_df.Filter(selection_expr);
        }
        variation_datasets.emplace(variation_type,
                                   SampleDataset{sample_def.sample_origin_,
                                                 AnalysisRole::kVariation,
                                                 variation_df});
      }

      SampleDataset nominal_dataset{sample_def.sample_origin_,
                                    AnalysisRole::kNominal, region_df};
      SampleDatasetGroup ensemble{std::move(nominal_dataset),
                                  std::move(variation_datasets)};

      if (sample_def.isData()) {
        sample_processors[sample_key] =
            std::make_unique<DataProcessor>(std::move(ensemble.nominal_));
      } else {
        monte_carlo_nodes.emplace(sample_key, region_df);
        sample_processors[sample_key] = std::make_unique<MonteCarloProcessor>(
            sample_key, std::move(ensemble));
      }
    }

    log::info("AnalysisRunner::run", "Sample processing sequence complete.");

    return {std::move(sample_processors), std::move(monte_carlo_nodes)};
  }

  std::vector<std::string>
  buildCumulativeFilters(const std::vector<std::string> &clauses) {
    std::vector<std::string> filters{""};

    std::string current;

    for (const auto &clause : clauses) {
      if (!current.empty())
        current += " && ";

      current += clause;

      filters.push_back(current);
    }

    void calculateWeightsPerStage(const ROOT::RDF::RNode &base_df,
                                  const std::vector<std::string> &cumulative_filters,
                                  std::vector<RegionAnalysis::StageCount> &stage_counts,
                                  const std::vector<std::string> &schemes,
                                  const std::unordered_map<std::string, std::vector<int>> &scheme_keys) {
        for (size_t i = 0; i < cumulative_filters.size(); ++i) {
            auto df = base_df;
    return filters;
  }

  void updateSchemeTallies(
      ROOT::RDF::RNode df, const std::vector<std::string> &schemes,
      const std::unordered_map<std::string, std::vector<int>> &scheme_keys,
      RegionAnalysis::StageCount &stage_count) {
    for (auto &s : schemes)
      for (int key : scheme_keys.at(s)) {
        auto ch_df = df.Filter(s + " == " + std::to_string(key));

        auto ch_w = ch_df.Sum<double>("nominal_event_weight");
        auto ch_w2 = ch_df.Sum<double>("w2");

        stage_count.schemes[s][key].first += ch_w.GetValue();
        stage_count.schemes[s][key].second += ch_w2.GetValue();
      }
  }

  void calculateWeightsPerStage(
      const ROOT::RDF::RNode &base_df,
      const std::vector<std::string> &cumulative_filters,
      std::vector<RegionAnalysis::StageCount> &stage_counts,
      const std::vector<std::string> &schemes,
      const std::unordered_map<std::string, std::vector<int>> &scheme_keys) {
    for (size_t i = 0; i < cumulative_filters.size(); ++i) {
      auto df = base_df;

      if (!cumulative_filters[i].empty())
        df = df.Filter(cumulative_filters[i]);

      auto tot_w = df.Sum<double>("nominal_event_weight");
      auto tot_w2 = df.Sum<double>("w2");

      stage_counts[i].total += tot_w.GetValue();
      stage_counts[i].total_w2 += tot_w2.GetValue();

      this->updateSchemeTallies(df, schemes, scheme_keys, stage_counts[i]);
    }
  }

  void computeCutFlow(const RegionHandle &region_handle,
                      RegionAnalysis &region_analysis) {
    auto &sample_frames = data_loader_.getSampleFrames();

    auto clauses = analysis_definition_.regionClauses(region_handle.key_);

    auto cumulative_filters = this->buildCumulativeFilters(clauses);

    std::vector<RegionAnalysis::StageCount> stage_counts(
        cumulative_filters.size());

    StratifierRegistry strat_reg;
    std::vector<std::string> schemes{"inclusive_strange_channels",
                                     "exclusive_strange_channels"};
    std::unordered_map<std::string, std::vector<int>> scheme_keys;

    for (auto &s : schemes)
      for (auto &k : strat_reg.getAllStratumKeysForScheme(s))
        scheme_keys[s].push_back(std::stoi(k.str()));

    void processVariables(const RegionHandle &region_handle, RegionAnalysis &region_analysis,
                          std::unordered_map<SampleKey, std::unique_ptr<ISampleProcessor>> &sample_processors,
                          std::unordered_map<SampleKey, ROOT::RDF::RNode> &monte_carlo_nodes) {
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
                                   [&](auto &p) { p.second->book(*histogram_factory_, binning, model); });

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

            AnalysisResult::printSummary(result);

            region_analysis.addFinalVariable(var_key, std::move(result));

            log::info("AnalysisRunner::run", "Variable pipeline concluded (", var_index, "/", var_total,
                      "):", var_key.str());
        }
    for (auto &[skey, sample_def] : sample_frames) {
      if (!sample_def.isMc())
        continue;

      auto base_df = sample_def.nominal_node_.Define(
          "w2", "nominal_event_weight*nominal_event_weight");

      this->calculateWeightsPerStage(base_df, cumulative_filters, stage_counts,
                                     schemes, scheme_keys);
    }

    region_analysis.setCutFlow(std::move(stage_counts));
  }

  bool isSampleEligible(const SampleKey &sample_key,
                        const RunConfig *run_config,
                        const std::string &region_beam,
                        const std::vector<std::string> &region_runs) const {
    static_cast<void>(sample_key);

    if (!run_config)
      return false;
    if (!region_beam.empty() && run_config->beam_mode != region_beam)
      return false;
    if (!region_runs.empty() &&
        std::find(region_runs.begin(), region_runs.end(),
                  run_config->run_period) == region_runs.end())
      return false;

    return true;
  }

  void processVariables(
      const RegionHandle &region_handle, RegionAnalysis &region_analysis,
      std::unordered_map<SampleKey, std::unique_ptr<ISampleProcessor>>
          &sample_processors,
      std::unordered_map<SampleKey, ROOT::RDF::RNode> &monte_carlo_nodes) {
    log::info("AnalysisRunner::run",
              "Iterating across observable variables...");
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

      log::info("AnalysisRunner::run", "Deploying variable pipeline (",
                var_index, "/", var_total, "):", var_key.str());
      log::info("AnalysisRunner::run", "Executing sample processors...");
      tbb::parallel_for_each(
          sample_processors.begin(), sample_processors.end(), [&](auto &p) {
            p.second->book(*histogram_factory_, binning, model);
          });

      log::info("AnalysisRunner::run", "Registering systematic variations...");
      tbb::parallel_for_each(monte_carlo_nodes.begin(), monte_carlo_nodes.end(),
                             [&](auto &p) {
                               systematics_processor_.bookSystematics(
                                   p.first, p.second, binning, model);
                             });

      log::info("AnalysisRunner::run", "Persisting results...");
      tbb::parallel_for_each(sample_processors.begin(), sample_processors.end(),
                             [&](auto &p) { p.second->contributeTo(result); });

      log::info("AnalysisRunner::run", "Computing systematic covariances");
      systematics_processor_.processSystematics(result);
      systematics_processor_.clearFutures();

      AnalysisResult::printSummary(result);

      region_analysis.addFinalVariable(var_key, std::move(result));

      log::info("AnalysisRunner::run", "Variable pipeline concluded (",
                var_index, "/", var_total, "):", var_key.str());
    }
  }

  AnalysisPluginManager plugin_manager_;
  SelectionRegistry selection_registry_;

  AnalysisDataLoader &data_loader_;
  AnalysisDefinition analysis_definition_;
  SystematicsProcessor &systematics_processor_;

  std::unique_ptr<HistogramFactory> histogram_factory_;
};

} // namespace analysis

#endif
