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
#include "Selection.h"
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
    Selection filter;
};

class AnalysisDefinition {
public:
    AnalysisDefinition(const SelectionRegistry& sel_reg,
                       const EventVariableRegistry& var_reg)
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
        auto valid = EventVariableRegistry::eventVariables(SampleType::kMonteCarlo);
        if (std::find(valid.begin(), valid.end(), expr) == valid.end()) {
            log::fatal("AnalysisDefinition", "unknown expression:", expr);
        }
        log::info("AnalysisDefinition", "adding variable:", key);
        variables_.emplace(key, VariableConfig{expr, lbl, strat, bdef});
        return *this;
    }

    AnalysisDefinition& addRegion(const std::string& key,
                                  const std::string& region_name,
                                  std::strong sel_rule_key)
    {
        if (regions_.count(key)) {
            log::fatal("AnalysisDefinition", "duplicate region:", key);
        }
        Selection sel = sel_reg_.get(sel_rule_key);
        regions_.emplace(
            key,
            RegionConfig{region_name, std::move(sel)}
        );
        return *this;
    }

    AnalysisDefinition& addRegionExpr(
        std::string key,
        std::string label,
        std::string rawExpr
    ) {
        if (regions_.count(key))
            throw std::runtime_error("duplicate region: "+key);
        regions_.emplace(
            key,
            RegionConfig{label, Selection(std::move(rawExpr))}
        );
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
    const EventVariableRegistry& var_reg_;          

    std::map<std::string, VariableConfig> variables_;   
    std::map<std::string, RegionConfig> regions_;       
};

}

#endif // ANALYSIS_DEFINITION_H



