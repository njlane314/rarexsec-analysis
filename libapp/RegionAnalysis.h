#pragma once

#include "AnalysisTypes.h"
#include "KeyTypes.h"

#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace analysis {

class RegionAnalysis {
  public:
    RegionAnalysis(RegionKey rk = RegionKey{}, std::string lbl = "", double pot = 0.0, bool bl = true,
                   std::string bc = {}, std::vector<std::string> runs = {})
        : region_key_(std::move(rk)), region_label_(std::move(lbl)), protons_on_target_(pot), is_blinded_(bl),
          beam_config_(std::move(bc)), run_numbers_(std::move(runs)) {}

    ~RegionAnalysis() = default;

    const RegionKey &regionKey() const noexcept { return region_key_; }
    const std::string &regionLabel() const noexcept {
        return region_label_.empty() ? region_key_.str() : region_label_;
    }
    double protonsOnTarget() const noexcept { return protons_on_target_; }
    void setProtonsOnTarget(double pot) noexcept { protons_on_target_ = pot; }
    void addProtonsOnTarget(double pot) noexcept { protons_on_target_ += pot; }
    bool isBlinded() const noexcept { return is_blinded_; }
    const std::string &beamConfig() const noexcept { return beam_config_; }
    const std::vector<std::string> &runNumbers() const noexcept { return run_numbers_; }

    void addFinalVariable(VariableKey v, VariableResult r) {
        final_variables_.insert_or_assign(std::move(v), std::move(r));
    }

    bool hasFinalVariable(const VariableKey &v) const { return final_variables_.find(v) != final_variables_.end(); }

    const VariableResult &getFinalVariable(const VariableKey &v) const {
        auto it = final_variables_.find(v);
        if (it == final_variables_.end()) {
            throw std::runtime_error("Final variable not found in RegionAnalysis: " + v.str());
        }
        return it->second;
    }

    std::vector<VariableKey> getAvailableVariables() const {
        std::vector<VariableKey> variables;
        variables.reserve(final_variables_.size());
        for (const auto &kv : final_variables_) {
            variables.emplace_back(kv.first);
        }
        return variables;
    }

    const std::map<VariableKey, VariableResult> &finalVariables() const noexcept { return final_variables_; }

  private:
    RegionKey region_key_{};
    std::string region_label_{};
    double protons_on_target_{0.0};
    bool is_blinded_{true};
    std::string beam_config_{};
    std::vector<std::string> run_numbers_{};
    std::map<VariableKey, VariableResult> final_variables_;
};

  } // namespace analysis
