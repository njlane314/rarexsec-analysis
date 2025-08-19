#ifndef REGION_ANALYSIS_H
#define REGION_ANALYSIS_H

#include "TObject.h"
#include "BinDefinition.h"
#include "Keys.h"
#include "SystematicsProcessor.h"
#include "BinnedHistogram.h"
#include "ROOT/RDataFrame.hxx"
#include <TH1D.h>
#include <map>
#include <vector>
#include <string>
#include <memory>
#include <stdexcept>

namespace analysis {

using TH1DFuture = ROOT::RDF::RResultPtr<TH1D>;

struct VariableFutures {
    BinDefinition bin_def;
    TH1DFuture data_future;
    TH1DFuture total_mc_future;
    std::unordered_map<int, TH1DFuture> stratified_mc_futures;
    SystematicFutures systematic_variations;
};

struct VariableResults {
    BinDefinition bin_def;
    BinnedHistogram data_hist;
    BinnedHistogram total_mc_hist;
    std::map<int, BinnedHistogram> stratified_mc_hists;
    std::map<std::string, std::map<std::string, std::map<int, BinnedHistogram>>> variation_hists;
    std::map<int, std::map<std::string, TMatrixDSym>> covariance_matrices;
};


class RegionAnalysis : public TObject {
public:
    RegionAnalysis(RegionKey region_key = RegionKey{},
                   double pot = 0.0,
                   bool is_blinded = true,
                   std::string beam_config = "",
                   std::vector<std::string> run_numbers = {})
        : region_key_(std::move(region_key))
        , protons_on_target_(pot)
        , is_blinded_(is_blinded)
        , beam_config_(std::move(beam_config))
        , run_numbers_(std::move(run_numbers))
    {}

    void addFinalVariable(const VariableKey& variable, VariableResults results) {
        final_variables_[variable] = std::move(results);
    }

    const VariableResults& getFinalVariable(const VariableKey& variable) const {
        auto it = final_variables_.find(variable);
        if (it == final_variables_.end()) {
            throw std::runtime_error("Final variable not found in region analysis: " + variable.str());
        }
        return it->second;
    }

    std::vector<VariableKey> getAvailableVariables() const {
        std::vector<VariableKey> variables;
        variables.reserve(final_variables_.size());
        for (const auto& [variable_key, _] : final_variables_) {
            variables.emplace_back(variable_key);
        }
        return variables;
    }

    std::map<VariableKey, VariableFutures> futures_;

private:
    RegionKey region_key_;
    std::map<VariableKey, VariableResults> final_variables_;
    double protons_on_target_;
    bool is_blinded_;
    std::string beam_config_;
    std::vector<std::string> run_numbers_;

    ClassDef(RegionAnalysis, 1);
};

}

#endif