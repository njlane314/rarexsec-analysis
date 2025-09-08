#ifndef CUT_FLOW_CALCULATOR_H
#define CUT_FLOW_CALCULATOR_H

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
    auto cumulative_filters = buildCumulativeFilters(clauses);

    std::vector<RegionAnalysis::StageCount> stage_counts(
        cumulative_filters.size());

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

    for (auto &[skey, sample_def] : sample_frames) {
      if (!sample_def.isMc()) {
        continue;
      }

      auto base_df = sample_def.nominal_node_.Define(
          "w2", "nominal_event_weight*nominal_event_weight");

      calculateWeightsPerStage(base_df, cumulative_filters, stage_counts,
                               schemes, scheme_keys, scheme_filters);
    }

    region_analysis.setCutFlow(std::move(stage_counts));
  }

  static std::vector<std::string>
  buildCumulativeFilters(const std::vector<std::string> &clauses) {
    const auto expected_size = clauses.size() + 1;
    std::vector<std::string> filters;
    filters.reserve(expected_size);
    filters.emplace_back("");
    std::string current;

    for (const auto &clause : clauses) {
      if (!current.empty()) {
        current += " && ";
      }

      current += clause;
      filters.push_back(current);
    }

    return filters;
  }

private:
  void updateSchemeTallies(
      ROOT::RDF::RNode df, const std::vector<std::string> &schemes,
      const std::unordered_map<std::string, std::vector<int>> &scheme_keys,
      const std::unordered_map<std::string,
                               std::unordered_map<int, std::string>>
          &scheme_filters,
      RegionAnalysis::StageCount &stage_count) {
    for (const auto &scheme : schemes) {
      for (int key : scheme_keys.at(scheme)) {
        auto ch_df = df.Filter(scheme_filters.at(scheme).at(key));

        auto ch_w = ch_df.Sum<double>("nominal_event_weight");
        auto ch_w2 = ch_df.Sum<double>("w2");

        stage_count.schemes[scheme][key].first += ch_w.GetValue();
        stage_count.schemes[scheme][key].second += ch_w2.GetValue();
      }
    }
  }

  void calculateWeightsPerStage(
      const ROOT::RDF::RNode &base_df,
      const std::vector<std::string> &cumulative_filters,
      std::vector<RegionAnalysis::StageCount> &stage_counts,
      const std::vector<std::string> &schemes,
      const std::unordered_map<std::string, std::vector<int>> &scheme_keys,
      const std::unordered_map<std::string,
                               std::unordered_map<int, std::string>>
          &scheme_filters) {
    for (size_t i = 0; i < cumulative_filters.size(); ++i) {
      auto df = base_df;

      if (!cumulative_filters[i].empty()) {
        df = df.Filter(cumulative_filters[i]);
      }

      auto tot_w = df.Sum<double>("nominal_event_weight");
      auto tot_w2 = df.Sum<double>("w2");

      stage_counts[i].total += tot_w.GetValue();
      stage_counts[i].total_w2 += tot_w2.GetValue();

      updateSchemeTallies(df, schemes, scheme_keys, scheme_filters,
                          stage_counts[i]);
    }
  }

  Loader &data_loader_;
  AnalysisDefinition &analysis_definition_;
};

} // namespace analysis

#endif
