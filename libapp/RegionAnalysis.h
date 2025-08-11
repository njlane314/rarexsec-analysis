#ifndef REGION_ANALYSIS_H
#define REGION_ANALYSIS_H

#include "TObject.h"
#include "BinDefinition.h"
#include "DeferredBinnedHistogram.h"
#include "Keys.h"
#include "SystematicsProcessor.h"
#include "ROOT/RDataFrame.hxx"
#include <TH1D.h>
#include <map>
#include <vector>
#include <string>
#include <memory>

namespace analysis {

struct VariableHistogramFutures {
    BinDefinition bin_def;
    std::string tex_label;
    
    ROOT::RDF::RResultPtr<TH1D> total_data_future;
    ROOT::RDF::RResultPtr<TH1D> total_mc_future;
    std::map<StratumKey, ROOT::RDF::RResultPtr<TH1D>> stratified_mc_futures;
    SystematicFutures systematic_variations;
};

struct MaterialisedHistograms {
    BinDefinition bin_def;
    std::string tex_label;
    VariableKey variable;
    
    DeferredBinnedHistogram data;
    DeferredBinnedHistogram total_monte_carlo;
    std::map<StratumKey, DeferredBinnedHistogram> channels;
    std::map<SystematicKey, TMatrixDSym> systematic_covariances;
    std::map<SystematicKey, std::map<VariationKey, DeferredBinnedHistogram>> systematic_variations;
    
    double pot = 0.0;
    bool blinded = true;
    std::string beam_configuration;
    std::vector<std::string> run_numbers;
};

class RegionAnalysis : public TObject {
public:
    RegionAnalysis(RegionKey region_key,
                   double pot = 0.0,
                   bool is_blinded = true,
                   std::string beam_config = "",
                   std::vector<std::string> runs = {})
        : region_key_(std::move(region_key))
        , pot_(pot)
        , is_blinded_(is_blinded)
        , beam_configuration_(std::move(beam_config))
        , run_numbers_(std::move(runs))
    {}
    
    void addVariableHistograms(const VariableKey& variable, VariableHistogramFutures histograms) {
        variable_histograms_[variable] = std::move(histograms);
    }
    
    MaterialisedHistograms materialiseVariable(const VariableKey& variable,
                                               SystematicsProcessor& systematic_processor) const {
        const auto variable_iterator = variable_histograms_.find(variable);
        if (variable_iterator == variable_histograms_.end()) {
            throw std::runtime_error("Variable not found in region analysis: " + variable.str());
        }
        
        return materialiseHistograms(variable, variable_iterator->second, systematic_processor);
    }
    
    std::vector<VariableKey> getAvailableVariables() const {
        std::vector<VariableKey> variables;
        variables.reserve(variable_histograms_.size());
        
        for (const auto& [variable_key, _] : variable_histograms_) {
            variables.emplace_back(variable_key);
        }
        
        return variables;
    }
    
    const RegionKey& getRegion() const { return region_key_; }
    bool hasVariable(const VariableKey& variable) const {
        return variable_histograms_.contains(variable);
    }
    
    void scale(double scaling_factor) {
        pot_ *= scaling_factor;
    }
    
private:
    MaterialisedHistograms materialiseHistograms(const VariableKey& variable,
                                                 const VariableHistogramFutures& futures,
                                                 SystematicsProcessor& systematic_processor) const {
        MaterialisedHistograms materialised{};
        materialised.variable = variable;
        materialised.bin_def = futures.bin_def;
        materialised.tex_label = futures.tex_label;
        materialised.pot = pot_;
        materialised.blinded = is_blinded_;
        materialised.beam_configuration = beam_configuration_;
        materialised.run_numbers = run_numbers_;
        
        if (futures.hasData()) {
            materialised.data = DeferredBinnedHistogram::fromFutureTH1D(futures.bin_def, futures.total_data_future);
        }
        
        if (futures.hasNominals()) {
            materialised.total_monte_carlo = DeferredBinnedHistogram::fromFutureTH1D(
                futures.bin_def, futures.total_mc_future
            );
        }
        
        for (const auto& [channel_key, channel_future] : futures.stratified_mc_futures) {
            materialised.channels[channel_key] = DeferredBinnedHistogram::fromFutureTH1D(
                futures.bin_def, channel_future
            );
        }
        
        if (futures.hasSystematics()) {
            // Materialize systematics through processor
            // This would involve calling the systematic processor methods we redesigned
        }
        
        return materialised;
    }
    
    RegionKey region_key_;
    std::map<VariableKey, VariableHistogramFutures> variable_histograms_;
    
    double pot_;
    bool is_blinded_;
    std::string beam_configuration_;
    std::vector<std::string> run_numbers_;
    
    ClassDef(RegionAnalysis, 1);
};

using HistogramResult = MaterialisedHistograms;

}

#endif