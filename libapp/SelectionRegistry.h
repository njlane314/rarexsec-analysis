#ifndef SELECTION_REGISTRY_H
#define SELECTION_REGISTRY_H

#include <string>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <stdexcept>

#include "Selection.h"
#include "AnalysisDefinition.h"

namespace analysis {

struct SelectionRule {
    std::string display_name;
    std::string short_key;
    std::vector<std::string> clauses;
};

class SelectionRegistry {
public:
    SelectionRegistry() {
        this->registerDefaults();
    }

    SelectionRegistry& addRule(const std::string& key, SelectionRule rule) {
        if (!rules_.count(key)) {
            rules_.emplace(key, std::move(rule));
        }
        return *this;
    }

    bool containsRule(const std::string& key) const noexcept {
        return rules_.count(key) > 0;
    }

    const SelectionRule& getRule(const std::string& key) const {
        auto it = rules_.find(key);
        if (it == rules_.end()) {
            throw std::out_of_range("unknown selection rule: " + key);
        }
        return it->second;
    }

    Selection getRegionFilterQuery(const RegionConfig& region) const {
        Selection query;
        for (const auto& key : region.selection_keys) {
            const auto& rule = getRule(key);
            query = query && Selection(makeExpression(rule));
        }
        return query;
    }

private:
    std::string makeExpression(const SelectionRule& rule) const {
        if (rule.clauses.empty()) {
            return "1";  
        }
        std::ostringstream oss;
        for (size_t i = 0; i < rule.clauses.size(); ++i) {
            if (i) oss << " && ";
            oss << rule.clauses[i];
        }
        return oss.str();
    }

    void registerDefaults() {
        rules_.emplace("QUALITY", SelectionRule{
            "Quality Preselection", "QUALITY", {"quality_selector"}
        });
        rules_.emplace("NUMU_CC", SelectionRule{
            "NuMu CC Selection", "NUMU_CC",
            {"muon_candidate_selector", "n_pfp_gen_2 > 1"}
        });
        rules_.emplace("ALL_EVENTS", SelectionRule{
            "All Events", "ALL_EVENTS", {}
        });
        rules_.emplace("NONE", SelectionRule{
            "No Preselection", "NONE", {}
        });
    }

    std::unordered_map<std::string, SelectionRule> rules_;
};

} 

#endif // SELECTION_REGISTRY_H
