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

    std::vector<RegionAnalysis::StageCount> stage_counts(clauses.size() + 1);

    StratifierRegistry strat_reg;
    const std::vector<std::string> schemes{"inclusive_strange_channels",
                                           "exclusive_strange_channels",
                                           "channel_definitions"};
    std::unordered_map<std::string, std::vector<int>> scheme_keys;
    std::unordered_map<std::string, std::unordered_map<int, std::string>>
        scheme_filters;

    for (const auto &scheme : schemes) {
      for (const auto &key : strat_reg.getAllStratumKeysForScheme(scheme)) {
        int int_key = std::stoi(key.str());
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

      auto cumulative_nodes =
          buildCumulativeFilters(base_df, clauses);

      calculateWeightsPerStage(cumulative_nodes, stage_counts, schemes,
                               scheme_keys, scheme_filters);
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
      const std::vector<ROOT::RDF::RNode> &cumulative_nodes,
      std::vector<RegionAnalysis::StageCount> &stage_counts,
      const std::vector<std::string> &schemes,
      const std::unordered_map<std::string, std::vector<int>> &scheme_keys,
      const std::unordered_map<std::string,
                               std::unordered_map<int, std::string>>
          &scheme_filters) {
    for (size_t i = 0; i < cumulative_nodes.size(); ++i) {
      auto df = cumulative_nodes[i];

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
