#ifndef ANALYSIS_DEFINITION_H
#define ANALYSIS_DEFINITION_H

#include <algorithm>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "AnalysisDataLoader.h"
#include "AnalysisKey.h"
#include "BinningDefinition.h"
#include "DynamicBinning.h"
#include "Logger.h"
#include "RegionAnalysis.h"
#include "RegionHandle.h"
#include "SelectionQuery.h"
#include "SelectionRegistry.h"
#include "VariableHandle.h"
#include "VariableRegistry.h"

namespace analysis {

class AnalysisDefinition {
public:
  AnalysisDefinition(const SelectionRegistry &sel_reg) : sel_reg_(sel_reg) {}

  AnalysisDefinition &addVariable(
      const std::string &key, const std::string &expr, const std::string &lbl,
      const BinningDefinition &bdef, const std::string &strat,
      bool is_dynamic = false, bool include_oob_bins = false,
      DynamicBinningStrategy strategy = DynamicBinningStrategy::EqualWeight,
      double bin_resolution = 0.0) {
    VariableKey var_key{key};
    this->ensureVariableUnique(var_key, key);
    this->validateExpression(expr);

    variable_expressions_.emplace(var_key, expr);
    variable_labels_.emplace(var_key, lbl);
    variable_binning_.emplace(var_key, bdef);
    variable_stratifiers_.emplace(var_key, strat);
    is_dynamic_.emplace(var_key, is_dynamic);
    include_oob_.emplace(var_key, include_oob_bins);
    dynamic_strategy_.emplace(var_key, strategy);
    dynamic_resolution_.emplace(var_key, bin_resolution);

    return *this;
  }

  AnalysisDefinition &
  addRegion(const std::string &key, const std::string &region_name,
            std::string sel_rule_key, double pot = 0.0, bool blinded = true,
            std::string beam_config = "", std::vector<std::string> runs = {}) {
    RegionKey region_key{key};
    this->ensureRegionUnique(region_key, key);

    auto rule = sel_reg_.getRule(sel_rule_key);
    SelectionQuery sel = sel_reg_.get(sel_rule_key);
    region_names_.emplace(region_key, region_name);
    region_selections_.emplace(region_key, std::move(sel));
    region_clauses_.emplace(region_key, rule.clauses);

    region_analyses_.emplace(
        region_key,
        this->makeRegionAnalysis(region_key, region_name, pot, blinded,
                                 std::move(beam_config), std::move(runs)));
    return *this;
  }

  AnalysisDefinition &addRegionExpr(const std::string &key,
                                    const std::string &label,
                                    std::string raw_expr, double pot = 0.0,
                                    bool blinded = true,
                                    std::string beam_config = "",
                                    std::vector<std::string> runs = {}) {
    RegionKey region_key{key};
    this->ensureRegionUnique(region_key, key);

    region_names_.emplace(region_key, label);
    region_selections_.emplace(region_key, SelectionQuery(std::move(raw_expr)));
    region_clauses_.emplace(region_key, std::vector<std::string>{});

    region_analyses_.emplace(
        region_key,
        this->makeRegionAnalysis(region_key, label, pot, blinded,
                                 std::move(beam_config), std::move(runs)));
    return *this;
  }

  void addVariableToRegion(const std::string &reg_key,
                           const std::string &var_key) {
    RegionKey region_key{reg_key};
    VariableKey variable_key{var_key};

    this->requireRegionExists(region_key, reg_key);
    this->requireVariableExists(variable_key, var_key);

    region_variables_[region_key].emplace_back(variable_key);
  }

  RegionHandle region(const RegionKey &key) const {
    return RegionHandle{key, region_names_, region_selections_,
                        region_analyses_, region_variables_};
  }

  bool includeOobBins(const VariableKey &key) const {
    auto it = include_oob_.find(key);
    return it != include_oob_.end() && it->second;
  }

  const std::vector<std::string> &regionClauses(const RegionKey &key) const {
    static const std::vector<std::string> empty;
    auto it = region_clauses_.find(key);
    return it != region_clauses_.end() ? it->second : empty;
  }

  std::vector<RegionHandle> regions() const {
    std::vector<RegionHandle> result;
    result.reserve(region_names_.size());

    for (const auto &entry : region_names_)
      result.emplace_back(entry.first, region_names_, region_selections_,
                          region_analyses_, region_variables_);

    return result;
  }

  void setBinning(const VariableKey &key, BinningDefinition &&bdef) {
    variable_binning_[key] = std::move(bdef);
  }

  VariableHandle variable(const VariableKey &key) const {
    return VariableHandle{key, variable_expressions_, variable_labels_,
                          variable_binning_, variable_stratifiers_};
  }

  std::vector<VariableHandle> variables() const {
    std::vector<VariableHandle> result;
    result.reserve(variable_expressions_.size());

    for (const auto &entry : variable_expressions_)
      result.emplace_back(entry.first, variable_expressions_, variable_labels_,
                          variable_binning_, variable_stratifiers_);

    return result;
  }

  bool isDynamic(const VariableKey &key) const {
    auto it = is_dynamic_.find(key);
    return it != is_dynamic_.end() && it->second;
  }

  DynamicBinningStrategy dynamicBinningStrategy(const VariableKey &key) const {
    auto it = dynamic_strategy_.find(key);
    return it != dynamic_strategy_.end() ? it->second
                                         : DynamicBinningStrategy::EqualWeight;
  }

  double dynamicBinningResolution(const VariableKey &key) const {
    auto it = dynamic_resolution_.find(key);
    return it != dynamic_resolution_.end() ? it->second : 0.0;
  }

  void resolveDynamicBinning(AnalysisDataLoader &loader) {
    for (const auto &var_handle : this->variables()) {
      if (!this->isDynamic(var_handle.key_))
        continue;
      log::info(
          "AnalysisDefinition::resolveDynamicBinning",
          "Deriving dynamic bin schema for variable:", var_handle.key_.str());
      std::vector<ROOT::RDF::RNode> mc_nodes;
      for (auto &entry : loader.getSampleFrames()) {
        auto &sample_def = entry.second;
        if (sample_def.isMc()) {
          mc_nodes.emplace_back(sample_def.nominal_node_);
        }
      }

      if (mc_nodes.empty()) {
        log::warn("AnalysisDefinition::resolveDynamicBinning",
                  "Skipping dynamic binning for variable",
                  var_handle.key_.str(),
                  ": no Monte Carlo samples were found.");
        for (auto &entry : loader.getSampleFrames()) {
          log::warn("AnalysisDefinition::resolveDynamicBinning",
                    "Available sample:", entry.first.str());
        }
        continue;
      }

      bool include_oob = this->includeOobBins(var_handle.key_);
      auto strategy = this->dynamicBinningStrategy(var_handle.key_);
      double bin_res = this->dynamicBinningResolution(var_handle.key_);
      BinningDefinition new_bins = DynamicBinning::calculate(
          mc_nodes, var_handle.binning(), "nominal_event_weight", 400.0,
          include_oob, strategy, bin_res);

      log::info("AnalysisDefinition::resolveDynamicBinning",
                "--> Optimal bin count resolved:", new_bins.getBinNumber());

      this->setBinning(var_handle.key_, std::move(new_bins));
    }
  }

private:
  const SelectionRegistry &sel_reg_;

  std::map<VariableKey, std::string> variable_expressions_;
  std::map<VariableKey, std::string> variable_labels_;
  std::map<VariableKey, BinningDefinition> variable_binning_;
  std::map<VariableKey, std::string> variable_stratifiers_;
  std::map<VariableKey, bool> is_dynamic_;
  std::map<VariableKey, bool> include_oob_;
  std::map<VariableKey, DynamicBinningStrategy> dynamic_strategy_;
  std::map<VariableKey, double> dynamic_resolution_;

  std::map<RegionKey, std::string> region_names_;
  std::map<RegionKey, SelectionQuery> region_selections_;
  std::map<RegionKey, std::unique_ptr<RegionAnalysis>> region_analyses_;
  std::map<RegionKey, std::vector<VariableKey>> region_variables_;
  std::map<RegionKey, std::vector<std::string>> region_clauses_;

  bool hasRegion(const RegionKey &key) const {
    return region_analyses_.count(key) != 0;
  }
  bool hasVariable(const VariableKey &key) const {
    return variable_expressions_.count(key) != 0;
  }

  void ensureRegionUnique(const RegionKey &key,
                          const std::string &key_str) const {
    if (hasRegion(key))
      log::fatal("AnalysisDefinition", "duplicate region:", key_str);
  }

  void ensureVariableUnique(const VariableKey &key,
                            const std::string &key_str) const {
    if (hasVariable(key))
      log::fatal("AnalysisDefinition", "duplicate variable:", key_str);
  }

  void requireRegionExists(const RegionKey &key,
                           const std::string &key_str) const {
    if (!hasRegion(key))
      log::fatal("AnalysisDefinition", "region not found:", key_str);
  }

  void requireVariableExists(const VariableKey &key,
                             const std::string &key_str) const {
    if (!hasVariable(key))
      log::fatal("AnalysisDefinition", "variable not found:", key_str);
  }

  void validateExpression(const std::string &expr) const {
    const auto valid =
        VariableRegistry::eventVariables(SampleOrigin::kMonteCarlo);
    if (std::find(valid.begin(), valid.end(), expr) == valid.end())
      log::fatal("AnalysisDefinition", "unknown expression:", expr);
  }

  static std::unique_ptr<RegionAnalysis>
  makeRegionAnalysis(const RegionKey &key, const std::string &label, double pot,
                     bool blinded, std::string beam_config,
                     std::vector<std::string> runs) {
    return std::make_unique<RegionAnalysis>(
        key, label, pot, blinded, std::move(beam_config), std::move(runs));
  }
};
} // namespace analysis

#endif
