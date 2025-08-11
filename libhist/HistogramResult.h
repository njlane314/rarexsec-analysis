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
    BinDefinition binning;
    std::string axis_label;
    
    ROOT::RDF::RResultPtr<TH1D> data_future;
    ROOT::RDF::RResultPtr<TH1D> total_monte_carlo_future;
    std::map<ChannelKey, ROOT::RDF::RResultPtr<TH1D>> channel_futures;
    SystematicFutures systematic_variations;
    
    bool hasData() const { return static_cast<bool>(data_future); }
    bool hasNominals() const { return static_cast<bool>(total_monte_carlo_future); }
    bool hasSystematics() const { return !systematic_variations.variations.empty(); }
};

struct MaterialisedHistograms {
    BinDefinition binning;
    std::string axis_label;
    VariableKey variable;
    
    DeferredBinnedHistogram data;
    DeferredBinnedHistogram total_monte_carlo;
    std::map<ChannelKey, DeferredBinnedHistogram> channels;
    std::map<SystematicKey, TMatrixDSym> systematic_covariances;
    std::map<SystematicKey, std::map<VariationKey, DeferredBinnedHistogram>> systematic_variations;
    
    double protons_on_target = 0.0;
    bool blinded = true;
    std::string beam_configuration;
    std::vector<std::string> run_numbers;
    
    void scale(double scaling_factor) {
        data = data.scaled(scaling_factor);
        total_monte_carlo = total_monte_carlo.scaled(scaling_factor);
        for (auto& [channel_key, histogram] : channels) {
            histogram = histogram.scaled(scaling_factor);
        }
        for (auto& [systematic_key, covariance_matrix] : systematic_covariances) {
            covariance_matrix *= (scaling_factor * scaling_factor);
        }
        for (auto& [systematic_key, variation_map] : systematic_variations) {
            for (auto& [variation_key, histogram] : variation_map) {
                histogram = histogram.scaled(scaling_factor);
            }
        }
        protons_on_target *= scaling_factor;
    }
};

class RegionAnalysis : public TObject {
public:
    RegionAnalysis(RegionKey region_identifier,
                   double protons_on_target = 0.0,
                   bool is_blinded = true,
                   std::string beam_config = "",
                   std::vector<std::string> runs = {})
        : region_identifier_(std::move(region_identifier))
        , protons_on_target_(protons_on_target)
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
    
    const RegionKey& getRegion() const { return region_identifier_; }
    bool hasVariable(const VariableKey& variable) const {
        return variable_histograms_.contains(variable);
    }
    
    void scale(double scaling_factor) {
        protons_on_target_ *= scaling_factor;
    }
    
private:
    MaterialisedHistograms materialiseHistograms(const VariableKey& variable,
                                                 const VariableHistogramFutures& futures,
                                                 SystematicsProcessor& systematic_processor) const {
        MaterialisedHistograms materialised{};
        materialised.variable = variable;
        materialised.binning = futures.binning;
        materialised.axis_label = futures.axis_label;
        materialised.protons_on_target = protons_on_target_;
        materialised.blinded = is_blinded_;
        materialised.beam_configuration = beam_configuration_;
        materialised.run_numbers = run_numbers_;
        
        if (futures.hasData()) {
            materialised.data = DeferredBinnedHistogram::fromFutureTH1D(futures.binning, futures.data_future);
        }
        
        if (futures.hasNominals()) {
            materialised.total_monte_carlo = DeferredBinnedHistogram::fromFutureTH1D(
                futures.binning, futures.total_monte_carlo_future
            );
        }
        
        for (const auto& [channel_key, channel_future] : futures.channel_futures) {
            materialised.channels[channel_key] = DeferredBinnedHistogram::fromFutureTH1D(
                futures.binning, channel_future
            );
        }
        
        if (futures.hasSystematics()) {
            // Materialize systematics through processor
            // This would involve calling the systematic processor methods we redesigned
        }
        
        return materialised;
    }
    
    RegionKey region_identifier_;
    std::map<VariableKey, VariableHistogramFutures> variable_histograms_;
    
    double protons_on_target_;
    bool is_blinded_;
    std::string beam_configuration_;
    std::vector<std::string> run_numbers_;
    
    ClassDef(RegionAnalysis, 1);
};

// Legacy alias for backward compatibility during transition
using HistogramResult = MaterialisedHistograms;

}

#endif