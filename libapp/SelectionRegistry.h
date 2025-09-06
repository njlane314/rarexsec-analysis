#ifndef SELECTION_REGISTRY_H
#define SELECTION_REGISTRY_H

#include <numeric>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "Selection.h"

namespace analysis {

struct SelectionRule {
    std::string display_name;
    std::vector<std::string> clauses;
};

class SelectionRegistry {
  public:
    SelectionRegistry() { this->registerDefaults(); }

    void addRule(std::string key, SelectionRule rule) { rules_.emplace(std::move(key), std::move(rule)); }

    Selection get(const std::string &key) const {
        auto it = rules_.find(key);
        if (it == rules_.end())
            throw std::out_of_range("Unknown selection key: " + key);
        return makeSelection(it->second);
    }

    const SelectionRule &getRule(const std::string &key) const {
        auto it = rules_.find(key);
        if (it == rules_.end()) {
            throw std::out_of_range("unknown selection rule: " + key);
        }
        return it->second;
    }

  private:
    Selection makeSelection(const SelectionRule &r) const {
        if (r.clauses.empty())
            return Selection{};
        return std::accumulate(std::next(r.clauses.begin()), r.clauses.end(), Selection(r.clauses.front()),
                               [](Selection a, const std::string &b) { return a && Selection(b); });
    }

    void registerDefaults() {
        const std::vector<std::pair<std::string, SelectionRule>> defaults{
            {"QUALITY", {"Quality Preselection", {"quality_event"}}},

            {"QUALITY_BREAKDOWN",
             {"Quality Preselection Breakdown",
              {"in_reco_fiducial",
               "num_slices == 1",
               "selection_pass",
               "optical_filter_pe_beam > 20"}}},

            {"NUMU_CC", {"NuMu CC Selection", {"has_muon", "n_pfps_gen2 > 1"}}},

            {"NUMU_CC_BREAKDOWN",
             {"NuMu CC Selection Breakdown",
              {"muon_score",
               "muon_length",
               "has_muon",
               "n_pfps_gen2 > 1"}}},

            {"QUALITY_NUMU_CC",
             {"Quality + NuMu CC Selection",
              {"quality_event",
               "has_muon",
               "n_pfps_gen2 > 1"}}},

            {"QUALITY_NUMU_CC_BREAKDOWN",
             {"Quality + NuMu CC Selection Breakdown",
              {"in_reco_fiducial",
               "num_slices == 1",
               "selection_pass",
               "optical_filter_pe_beam > 20",
               "muon_score",
               "muon_length",
               "has_muon",
               "n_pfps_gen2 > 1"}}},

            {"ALL_EVENTS", {"All Events", {}}},
            {"NONE", {"No Preselection", {}}}};

        for (const auto &r : defaults)
            rules_.emplace(r);
    }

    std::unordered_map<std::string, SelectionRule> rules_;
};

}

#endif
