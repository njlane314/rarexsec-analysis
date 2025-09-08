#ifndef CUT_FLOW_CALCULATOR_H
#define CUT_FLOW_CALCULATOR_H

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include "AnalysisDataLoader.h"
#include "AnalysisDefinition.h"
#include "Logger.h"
#include "RegionAnalysis.h"
#include "RegionHandle.h"
#include "StratifierRegistry.h"

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
      if (!sample_def.isMc()) {
        log::debug("CutFlowCalculator::compute", skey.str(),
                   "is not MC - skipping");
        continue;
      }

      auto base_df = sample_def.nominal_node_.Define(
          "w2", "nominal_event_weight*nominal_event_weight");

      auto cumulative_nodes =
          buildCumulativeFilters(base_df, clauses);

      calculateWeightsPerStage(cumulative_nodes, stage_counts, schemes,
                               scheme_keys, scheme_filters);
      log::debug("CutFlowCalculator::compute", "Completed sample", skey.str());
    }

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

        value_setters.emplace_back(
            [&stage_count, scheme, key, ch_w]() mutable {
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
