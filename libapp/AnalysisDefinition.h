#ifndef ANALYSIS_DEFINITION_H
#define ANALYSIS_DEFINITION_H

#include <algorithm>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "AnalysisDataLoader.h"
#include "AnalysisLogger.h"
#include "BinningDefinition.h"
#include "DynamicBinning.h"
#include "AnalysisKey.h"
#include "RegionAnalysis.h"
#include "RegionHandle.h"
#include "Selection.h"
#include "SelectionRegistry.h"
#include "VariableHandle.h"
#include "VariableRegistry.h"

namespace analysis {
enum class DynamicBinningStrategy;
}

namespace analysis {

class AnalysisDefinition {
  public:
    AnalysisDefinition(const SelectionRegistry &sel_reg, const VariableRegistry &var_reg)
        : sel_reg_(sel_reg), var_reg_(var_reg) {}

    AnalysisDefinition &addVariable(const std::string &key, const std::string &expr, const std::string &lbl,
                                    const BinningDefinition &bdef, const std::string &strat, bool is_dynamic = false,
                                    bool include_out_of_range_bins = false,
                                    DynamicBinningStrategy strategy = DynamicBinningStrategy::EqualWeight) {
        VariableKey var_key{key};
        if (variable_expressions_.count(var_key))
            log::fatal("AnalysisDefinition", "duplicate variable:", key);

        auto valid = VariableRegistry::eventVariables(SampleOrigin::kMonteCarlo);
        if (std::find(valid.begin(), valid.end(), expr) == valid.end())
            log::fatal("AnalysisDefinition", "unknown expression:", expr);

        variable_expressions_[var_key] = expr;
        variable_labels_[var_key] = lbl;
        variable_binning_[var_key] = bdef;
        variable_stratifiers_[var_key] = strat;
        is_dynamic_[var_key] = is_dynamic;
        include_out_of_range_[var_key] = include_out_of_range_bins;
        dynamic_strategy_[var_key] = strategy;

        return *this;
    }

    AnalysisDefinition &addRegion(const std::string &key, const std::string &region_name, std::string sel_rule_key,
                                  double pot = 0.0, bool blinded = true, std::string beam_config = "",
                                  std::vector<std::string> runs = {}) {
        RegionKey region_key{key};
        if (region_analyses_.count(region_key))
            log::fatal("AnalysisDefinition", "duplicate region:", key);

        auto rule = sel_reg_.getRule(sel_rule_key);
        Selection sel = sel_reg_.get(sel_rule_key);
        region_names_[region_key] = region_name;
        region_selections_[region_key] = std::move(sel);
        region_clauses_[region_key] = rule.clauses;

        auto region_analysis = std::make_unique<RegionAnalysis>(region_key, region_name, pot, blinded,
                                                                std::move(beam_config), std::move(runs));

        region_analyses_[region_key] = std::move(region_analysis);
        return *this;
    }

    AnalysisDefinition &addRegionExpr(const std::string &key, const std::string &label, std::string raw_expr,
                                      double pot = 0.0, bool blinded = true, std::string beam_config = "",
                                      std::vector<std::string> runs = {}) {
        RegionKey region_key{key};
        if (region_analyses_.count(region_key))
            log::fatal("AnalysisDefinition", "duplicate region: " + key);

        region_names_[region_key] = label;
        region_selections_[region_key] = Selection(std::move(raw_expr));
        region_clauses_[region_key] = {};

        auto region_analysis =
            std::make_unique<RegionAnalysis>(region_key, label, pot, blinded, std::move(beam_config), std::move(runs));

        region_analyses_[region_key] = std::move(region_analysis);
        return *this;
    }

    void addVariableToRegion(const std::string &reg_key, const std::string &var_key) {
        RegionKey region_key{reg_key};
        VariableKey variable_key{var_key};

        if (!region_analyses_.count(region_key))
            log::fatal("AnalysisDefinition", "region not found:", reg_key);

        if (!variable_expressions_.count(variable_key))
            log::fatal("AnalysisDefinition", "variable not found:", var_key);

        region_variables_[region_key].emplace_back(variable_key);
    }

    bool isDynamic(const VariableKey &key) const {
        auto it = is_dynamic_.find(key);
        return it != is_dynamic_.end() && it->second;
    }

    bool includeOutOfRangeBins(const VariableKey &key) const {
        auto it = include_out_of_range_.find(key);
        return it != include_out_of_range_.end() && it->second;
    }

    DynamicBinningStrategy dynamicBinningStrategy(const VariableKey &key) const {
        auto it = dynamic_strategy_.find(key);
        return it != dynamic_strategy_.end() ? it->second : DynamicBinningStrategy::EqualWeight;
    }

    void setBinning(const VariableKey &key, BinningDefinition &&bdef) { variable_binning_[key] = std::move(bdef); }

    VariableHandle variable(const VariableKey &key) const {
        return VariableHandle{key, variable_expressions_, variable_labels_, variable_binning_, variable_stratifiers_};
    }

    RegionHandle region(const RegionKey &key) const {
        return RegionHandle{key, region_names_, region_selections_, region_analyses_, region_variables_};
    }

    const std::vector<std::string> &regionClauses(const RegionKey &key) const {
        static const std::vector<std::string> empty;
        auto it = region_clauses_.find(key);
        return it != region_clauses_.end() ? it->second : empty;
    }

    std::vector<VariableHandle> variables() const {
        std::vector<VariableHandle> result;
        result.reserve(variable_expressions_.size());

        for (const auto &entry : variable_expressions_)
            result.emplace_back(entry.first, variable_expressions_, variable_labels_, variable_binning_,
                                 variable_stratifiers_);

        return result;
    }

    std::vector<RegionHandle> regions() const {
        std::vector<RegionHandle> result;
        result.reserve(region_names_.size());

        for (const auto &entry : region_names_)
            result.emplace_back(entry.first, region_names_, region_selections_, region_analyses_, region_variables_);

        return result;
    }

    void resolveDynamicBinning(AnalysisDataLoader &loader) {
        for (const auto &var_handle : variables()) {
            if (!isDynamic(var_handle.key_))
                continue;
            log::info("AnalysisDefinition::resolveDynamicBinning",
                      "Deriving dynamic bin schema for variable:", var_handle.key_.str());
            std::vector<ROOT::RDF::RNode> mc_nodes;
            for (auto &[sample_key, sample_def] : loader.getSampleFrames()) {
                if (sample_def.isMc()) {
                    mc_nodes.emplace_back(sample_def.nominal_node_);
                }
            }

            if (mc_nodes.empty()) {
                log::fatal("AnalysisDefinition::resolveDynamicBinning",
                           "Cannot perform dynamic binning: No Monte Carlo samples were found!");
            }

            bool include_oob = includeOutOfRangeBins(var_handle.key_);
            auto strategy = dynamicBinningStrategy(var_handle.key_);
            BinningDefinition new_bins = DynamicBinning::calculate(
                mc_nodes, var_handle.binning(), "nominal_event_weight", 400.0, include_oob, strategy);

            log::info("AnalysisDefinition::resolveDynamicBinning",
                      "--> Optimal bin count resolved:", new_bins.getBinNumber());

            setBinning(var_handle.key_, std::move(new_bins));
        }
    }

  private:
    const SelectionRegistry &sel_reg_;
    const VariableRegistry &var_reg_;

    std::map<VariableKey, std::string> variable_expressions_;
    std::map<VariableKey, std::string> variable_labels_;
    std::map<VariableKey, BinningDefinition> variable_binning_;
    std::map<VariableKey, std::string> variable_stratifiers_;
    std::map<VariableKey, bool> is_dynamic_;
    std::map<VariableKey, bool> include_out_of_range_;
    std::map<VariableKey, DynamicBinningStrategy> dynamic_strategy_;

    std::map<RegionKey, std::string> region_names_;
    std::map<RegionKey, Selection> region_selections_;
    std::map<RegionKey, std::unique_ptr<RegionAnalysis>> region_analyses_;
    std::map<RegionKey, std::vector<VariableKey>> region_variables_;
    std::map<RegionKey, std::vector<std::string>> region_clauses_;
};

}

#endif
