#ifndef REGION_ANALYSIS_H
#define REGION_ANALYSIS_H

#include "TObject.h"
#include "BinDefinition.h"
#include "Keys.h"
#include "SystematicsProcessor.h"
#include "ROOT/RDataFrame.hxx"
#include <TH1D.h>
#include <map>
#include <vector>
#include <string>
#include <memory>
#include <stdexcept>

namespace analysis {

using TH1DFuture = ROOT::RDF::RResultPtr<TH1D>;

struct VariableHistogramFutures {
    BinDefinition bin_def;

    TH1DFuture total_data_future;

    TH1DFuture total_mc_future;
    std::unordered_map<StratumKey, TH1DFuture> stratified_mc_futures;
    std::unordered_map<VariationKey, TH1DFuture> variation_mc_futures;
};

class RegionAnalysis : public TObject {
public:
    RegionAnalysis(RegionKey region_key,
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

    void addVariableHistograms(const VariableKey& variable, VariableHistogramFutures histograms) {
        variable_histograms_[variable] = std::move(histograms);
    }

    const VariableHistogramFutures& getVariableFutures(const VariableKey& variable) const {
        auto it = variable_histograms_.find(variable);
        if (it == variable_histograms_.end()) {
            log::fatal("RegionAnalysis", "Variable not found in region analysis:", variable.str());
        }
        return it->second;
    }

    std::vector<VariableKey> getAvailableVariables() const {
        std::vector<VariableKey> variables;
        variables.reserve(variable_histograms_.size());
        for (const auto& [variable_key, _] : variable_histograms_) {
            variables.emplace_back(variable_key);
        }
        return variables;
    }

    const RegionKey& getRegionKey() const { return region_key_; }
    bool hasVariable(const VariableKey& variable) const {
        return variable_histograms_.count(variable);
    }

private:
    RegionKey region_key_;
    std::map<VariableKey, VariableHistogramFutures> variable_histograms_;

    double protons_on_target_;
    bool is_blinded_;
    std::string beam_config_;
    std::vector<std::string> run_numbers_;

    ClassDef(RegionAnalysis, 1);
};

}

#endif // REGION_ANALYSIS_H