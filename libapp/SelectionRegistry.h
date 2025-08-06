#ifndef SELECTION_REGISTRY_H
#define SELECTION_REGISTRY_H

#include <string>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <stdexcept>
#include <numeric>

#include "Selection.h"
#include "AnalysisDefinition.h"

namespace analysis {

struct SelectionRule {
    std::string display_name;
    std::vector<std::string> clauses;
};

class SelectionRegistry {
public:
    SelectionRegistry() {
        this->registerDefaults();
    }

    void addRule(std::string key, SelectionRule rule) {
        rules_.emplace(std::move(key), std::move(rule));
    }

    Selection get(const std::string& key) const {
        auto it = rules_.find(key);
        if (it == rules_.end())
            throw std::out_of_range("Unknown selection key: " + key);
        return makeSelection(it->second);
    }

    const SelectionRule& getRule(const std::string& key) const {
        auto it = rules_.find(key);
        if (it == rules_.end()) {
            throw std::out_of_range("unknown selection rule: " + key);
        }
        return it->second;
    }

private:
    Selection makeSelection(const SelectionRule& r) const {
        if (r.clauses.empty()) return Selection{};
            return std::accumulate(std::next(r.clauses.begin()), r.clauses.end(),
                                    Selection(r.clauses.front()),
                                    [](Selection a, const std::string& b){ return a && Selection(b); }
        );
    }

    void registerDefaults() {
        rules_.emplace("QUALITY", SelectionRule{
            "Quality Preselection", {"quality_event"}
        });
        rules_.emplace("NUMU_CC", SelectionRule{
            "NuMu CC Selection",
            {"has_muon", "n_pfps_gen2 > 1"}
        });
        rules_.emplace("ALL_EVENTS", SelectionRule{
            "All Events", {}
        });
        rules_.emplace("NONE", SelectionRule{
            "No Preselection", {}
        });
    }

    std::unordered_map<std::string, SelectionRule> rules_;
};

}

#endif // SELECTION_REGISTRY_H