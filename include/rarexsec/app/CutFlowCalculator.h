#ifndef CUT_FLOW_CALCULATOR_H
#define CUT_FLOW_CALCULATOR_H

#include <functional>
#include <iomanip>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include <rarexsec/data/AnalysisDataLoader.h>
#include <rarexsec/app/AnalysisDefinition.h>
#include <rarexsec/utils/Logger.h>
#include <rarexsec/app/RegionAnalysis.h>
#include <rarexsec/app/RegionHandle.h>
#include <rarexsec/hist/StratifierRegistry.h>

namespace analysis {

template <typename Loader> class CutFlowCalculator {
public:
  CutFlowCalculator(Loader &ldr, AnalysisDefinition &def)
      : data_loader_(ldr), analysis_definition_(def) {}

  void compute(const RegionHandle &region_handle,
               RegionAnalysis &region_analysis) {
    auto &sample_frames = data_loader_.getSampleFrames();
    auto clauses = analysis_definition_.regionClauses(region_handle.key_);

    std::vector<RegionAnalysis::StageCount> stage_counts(clauses.size() + 1);

    StratifierRegistry strat_reg;
    const std::vector<std::string> schemes{"inclusive_strange_channels",
                                           "exclusive_strange_channels",
                                           "channel_definitions"};
    std::unordered_map<std::string, std::vector<int>> scheme_keys;
    std::unordered_map<std::string, std::unordered_map<int, std::string>>
        scheme_filters;

    for (const auto &scheme : schemes) {
      for (int int_key : strat_reg.getAllStratumIntKeysForScheme(scheme)) {
        scheme_keys[scheme].push_back(int_key);
        scheme_filters[scheme][int_key] =
            scheme + " == " + std::to_string(int_key);
      }
    }

    log::debug("CutFlowCalculator::compute", "Processing", sample_frames.size(),
               "sample frames");
    for (auto &[skey, sample_def] : sample_frames) {
      log::debug("CutFlowCalculator::compute", "Examining sample", skey.str());

      auto base_df = sample_def.nominal_node_.Define(
          "w2", "nominal_event_weight*nominal_event_weight");

      auto cumulative_nodes = buildCumulativeFilters(base_df, clauses);

      calculateWeightsPerStage(cumulative_nodes, stage_counts, schemes,
                               scheme_keys, scheme_filters);
      log::debug("CutFlowCalculator::compute", "Completed sample", skey.str());
    }

    printSummary(region_handle, clauses, stage_counts);
    region_analysis.setCutFlow(std::move(stage_counts));
  }

  static std::vector<ROOT::RDF::RNode>
  buildCumulativeFilters(const ROOT::RDF::RNode &base_df,
                         const std::vector<std::string> &clauses) {
    std::vector<ROOT::RDF::RNode> nodes;
    nodes.reserve(clauses.size() + 1);
    nodes.push_back(base_df);

    auto current = base_df;
    for (const auto &clause : clauses) {
      current = current.Filter(clause);
      nodes.push_back(current);
    }

    return nodes;
  }

  static void
  printSummary(const RegionHandle &region_handle,
               const std::vector<std::string> &clauses,
               const std::vector<RegionAnalysis::StageCount> &stage_counts) {
    size_t width = 70;
    std::string line(width, '=');
    std::string sub(width, '-');

    std::cout << '\n' << line << '\n';
    std::cout << std::left << std::setw(width)
              << ("CutFlow Summary: " + region_handle.key_.str()) << '\n';
    std::cout << line << '\n';

    std::cout << std::fixed << std::setprecision(2);
    size_t stage_w = 30;
    size_t total_w = 20;
    size_t eff_w = 10;
    std::cout << std::left << std::setw(stage_w) << "Stage" << std::right
              << std::setw(total_w) << "Total MC"
              << std::setw(eff_w) << "Cum Eff"
              << std::setw(eff_w) << "Inc Eff" << '\n';

    double initial_total = stage_counts.empty() ? 0.0 : stage_counts[0].total;
    for (size_t i = 0; i < stage_counts.size(); ++i) {
      std::string label = i == 0 ? "initial" : clauses[i - 1];
      double cum_eff =
          initial_total != 0.0 ? stage_counts[i].total / initial_total : 0.0;
      double prev_total =
          i == 0 ? stage_counts[0].total : stage_counts[i - 1].total;
      double inc_eff =
          prev_total != 0.0 ? stage_counts[i].total / prev_total : 0.0;

      std::cout << std::left << std::setw(stage_w) << label << std::right
                << std::setw(total_w) << stage_counts[i].total
                << std::setw(eff_w) << cum_eff
                << std::setw(eff_w) << inc_eff << '\n';
    }

    std::cout << sub << '\n';
    std::cout << std::left << std::setw(width)
              << "Stratum MC Sums (final stage)" << '\n';

    if (!stage_counts.empty()) {
      const auto &final_stage = stage_counts.back();
      for (const auto &[scheme, m] : final_stage.schemes) {
        std::cout << std::left << std::setw(width) << scheme << '\n';
        for (const auto &[key, pr] : m) {
          std::cout << std::left << std::setw(stage_w) << key << std::right
                    << std::setw(width - stage_w) << pr.first << '\n';
        }
      }
    }

    std::cout << line << "\n";
  }

private:
  void updateSchemeTallies(
      ROOT::RDF::RNode df, const std::vector<std::string> &schemes,
      const std::unordered_map<std::string, std::vector<int>> &scheme_keys,
      const std::unordered_map<
          std::string, std::unordered_map<int, std::string>> &scheme_filters,
      RegionAnalysis::StageCount &stage_count,
      std::vector<ROOT::RDF::RResultPtr<double>> &results,
      std::vector<std::function<void()>> &value_setters) {
    for (const auto &scheme : schemes) {
      log::debug("CutFlowCalculator::updateSchemeTallies", "Scheme", scheme);
      for (int key : scheme_keys.at(scheme)) {
        log::debug("CutFlowCalculator::updateSchemeTallies", "  Key", key);
        auto ch_df = df.Filter(scheme_filters.at(scheme).at(key));

        auto ch_w = ch_df.Sum<double>("nominal_event_weight");
        auto ch_w2 = ch_df.Sum<double>("w2");

        stage_count.schemes[scheme][key];

        results.emplace_back(ch_w);
        results.emplace_back(ch_w2);

        value_setters.emplace_back([&stage_count, scheme, key, ch_w]() mutable {
          stage_count.schemes[scheme][key].first += ch_w.GetValue();
        });
        value_setters.emplace_back(
            [&stage_count, scheme, key, ch_w2]() mutable {
              stage_count.schemes[scheme][key].second += ch_w2.GetValue();
            });
      }
    }
  }

  void calculateWeightsPerStage(
      const std::vector<ROOT::RDF::RNode> &cumulative_nodes,
      std::vector<RegionAnalysis::StageCount> &stage_counts,
      const std::vector<std::string> &schemes,
      const std::unordered_map<std::string, std::vector<int>> &scheme_keys,
      const std::unordered_map<
          std::string, std::unordered_map<int, std::string>> &scheme_filters) {
    std::vector<ROOT::RDF::RResultPtr<double>> results;
    std::vector<std::function<void()>> value_setters;

    for (size_t i = 0; i < cumulative_nodes.size(); ++i) {
      auto df = cumulative_nodes[i];
      log::debug("CutFlowCalculator::calculateWeightsPerStage", "Stage", i);

      auto tot_w = df.Sum<double>("nominal_event_weight");
      auto tot_w2 = df.Sum<double>("w2");

      results.emplace_back(tot_w);
      results.emplace_back(tot_w2);

      value_setters.emplace_back([&stage_counts, i, tot_w]() mutable {
        stage_counts[i].total += tot_w.GetValue();
      });
      value_setters.emplace_back([&stage_counts, i, tot_w2]() mutable {
        stage_counts[i].total_w2 += tot_w2.GetValue();
      });

      updateSchemeTallies(df, schemes, scheme_keys, scheme_filters,
                          stage_counts[i], results, value_setters);
    }

    for (auto &res : results) {
      res.GetValue();
    }

    for (auto &setter : value_setters) {
      setter();
    }
  }

  Loader &data_loader_;
  AnalysisDefinition &analysis_definition_;
};

} // namespace analysis

#endif
