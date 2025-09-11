#ifndef SELECTION_REGISTRY_H
#define SELECTION_REGISTRY_H

#include <numeric>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include <rarexsec/core/SelectionQuery.h>

namespace analysis {

struct SelectionRule {
    std::string display_name;
    std::vector<std::string> clauses;
};

class SelectionRegistry {
  public:
    SelectionRegistry() { this->registerDefaults(); }

    void addRule(std::string key, SelectionRule rule) { rules_.emplace(std::move(key), std::move(rule)); }

    /// Return a SelectionQuery representing the rule identified by `key`.
    /// Throws std::out_of_range if the rule is not registered.
    SelectionQuery get(const std::string &key) const {
        const SelectionRule *rule = this->findRule(key);
        if (!rule)
            throw std::out_of_range("Unknown selection key: " + key);
        return this->makeSelection(*rule);
    }

    /// Retrieve the SelectionRule identified by `key` without constructing a
    /// SelectionQuery. Throws std::out_of_range if the rule is not registered.
    const SelectionRule &getRule(const std::string &key) const {
        const SelectionRule *rule = this->findRule(key);
        if (!rule)
            throw std::out_of_range("unknown selection rule: " + key);
        return *rule;
    }

  private:
    const SelectionRule *findRule(const std::string &key) const {
        auto it = rules_.find(key);
        return it == rules_.end() ? nullptr : &it->second;
    }

    SelectionQuery makeSelection(const SelectionRule &r) const {
        if (r.clauses.empty())
            return SelectionQuery{};
        return std::accumulate(std::next(r.clauses.begin()), r.clauses.end(), SelectionQuery(r.clauses.front()),
                               [](SelectionQuery a, const std::string &b) { return a && SelectionQuery(b); });
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

            {"MUON", {"Muon Selection", {"has_muon"}}},

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

            {"NUMUSEL",
             {"NuMu Selection",
              {"NUMUPRESEL",
               "n_muons_tot > 0"}}},

            {"NUMUSEL_CRT",
             {"NuMu Selection with CRT cuts",
              {"NUMUPRESEL",
               "n_muons_tot > 0",
               "(crtveto != 1 || crthitpe < 100)",
               "_closestNuCosmicDist > 5"}}},

            {"ALL_EVENTS", {"All Events", {}}},
            {"NONE", {"No Preselection", {}}}};

        for (const auto &r : defaults)
            rules_.emplace(r);
    }

    std::unordered_map<std::string, SelectionRule> rules_;
};

}

#endif
