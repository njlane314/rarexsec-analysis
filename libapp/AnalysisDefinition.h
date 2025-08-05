#ifndef ANALYSIS_DEFINITION_H
#define ANALYSIS_DEFINITION_H

#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <initializer_list>

#include "Logger.h"
#include "BinDefinition.h"
#include "EventVariableRegistry.h"
#include "SelectionRegistry.h"

namespace analysis {

struct VariableConfig {
    std::string branch_expression;
    std::string axis_label;
    std::string stratifier;
    BinDefinition bin_def;
};

struct RegionConfig {
    std::string region_name;
    std::vector<std::string> selection_keys;
};

class AnalysisDefinition {
public:
    AnalysisDefinition(const SelectionRegistry& sel_reg,
                       const VariableRegistry& var_reg)
      : sel_reg_(sel_reg), var_reg_(var_reg) 
    {
        log::info("AnalysisDefinition", "initialised");
    }

    AnalysisDefinition& addVariable(const std::string& key,
                                    const std::string& expr,
                                    const std::string& lbl,
                                    const BinDefinition& bdef,
                                    const std::string& strat)
    {
        if (variables_.count(key)) {
            log::fatal("AnalysisDefinition", "duplicate variable:", key);
        }
        auto valid = VariableRegistry::getEventVariables(SampleType::kMonteCarlo);
        if (std::find(valid.begin(), valid.end(), expr) == valid.end()) {
            log::fatal("AnalysisDefinition", "unknown expression:", expr);
        }
        log::info("AnalysisDefinition", "adding variable:", key);
        variables_.emplace(key, VariableConfig{expr, lbl, strat, bdef});
        return *this;
    }

    AnalysisDefinition& addRegion(const std::string& key,
                                  const std::string& region_name,
                                  std::initializer_list<std::string> selection_keys)
    {
        if (regions_.count(key)) {
            log::fatal("AnalysisDefinition", "duplicate region:", key);
        }
        for (const auto& sel_key : selection_keys) {
            if (!sel_reg_.containsRule(sel_key)) {
                log::fatal("AnalysisDefinition", "unknown selection key:", sel_key);
            }
        }
        log::info("AnalysisDefinition", "adding region:", key);
        regions_.emplace(key, RegionConfig{region_name, std::vector<std::string>(selection_keys)});
        return *this;
    }

    const std::map<std::string, VariableConfig>& getVariables() const noexcept {
        return variables_;
    }

    const std::map<std::string, RegionConfig>& getRegions() const noexcept {
        return regions_;
    }

private:
    const SelectionRegistry& sel_reg_;          
    const VariableRegistry& var_reg_;          

    std::map<std::string, VariableConfig> variables_;   
    std::map<std::string, RegionConfig> regions_;       
};

}

#endif // ANALYSIS_DEFINITION_H



