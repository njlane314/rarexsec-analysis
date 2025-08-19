#ifndef DATAFRAME_HISTOGRAM_BUILDER_H
#define DATAFRAME_HISTOGRAM_BUILDER_H

#include "HistogramDirector.h"
#include "StratifierFactory.h"
#include "StratificationRegistry.h"
#include "RegionAnalysis.h"
#include "ROOT/RDataFrame.hxx"
#include <TH1D.h>
#include <map>
#include <string>
#include <vector>
#include <unordered_map>
#include "Logger.h"
#include <regex>
#include "TypeKey.h"
#include "DeferredBinnedHistogram.h"

namespace analysis {

class DataFrameHistogramBuilder : public HistogramDirector {
public:
    DataFrameHistogramBuilder(StratificationRegistry& strat_reg)
      : stratification_registry_(strat_reg)
    {}

protected:
    void buildDataHistograms(const BinDefinition& variable_binning,
                             const SampleDataFrameMap& samples,
                             const ROOT::RDF::TH1DModel& histogram_model,
                             VariableHistogramFutures& variable_futures) override {
        std::vector<ROOT::RDF::RResultPtr<TH1D>> data_histogram_futures;
        
        for (const auto& [sample_identifier, sample_data] : samples) {
            if (sample_data.first == SampleType::kData) {
                data_histogram_futures.emplace_back(
                    sample_data.second.Histo1D(histogram_model, variable_binning.getVariable().Data())
                );
            }
        }
        
        if (!data_histogram_futures.empty()) {
            variable_futures.data_future = combineHistogramFutures(data_histogram_futures);
        }
    }

    void buildNominalHistograms(const BinDefinition& variable_binning,
                                const SampleDataFrameMap& samples,
                                const ROOT::RDF::TH1DModel& histogram_model,
                                VariableHistogramFutures& variable_futures) override {
        const auto histogram_stratifier = createStratifier(variable_binning);
        std::vector<ROOT::RDF::RResultPtr<TH1D>> monte_carlo_histogram_futures;
        
        for (const auto& [sample_identifier, sample_data] : samples) {
            if (sample_data.first != SampleType::kData) {
                const auto channel_histogram_futures = histogram_stratifier->bookNominalHistograms(
                    sample_data.second, variable_binning, histogram_model
                );
                
                for (const auto& [channel_identifier, histogram_future] : channel_histogram_futures) {
                    const ChannelKey channel_key{channel_identifier};
                    variable_futures.channel_futures[channel_key] = histogram_future;
                    monte_carlo_histogram_futures.emplace_back(histogram_future);
                }
            }
        }
        
        if (!monte_carlo_histogram_futures.empty()) {
            variable_futures.total_monte_carlo_future = combineHistogramFutures(monte_carlo_histogram_futures);
        }
    }

    void buildVariationHistograms(const BinDefinition& variable_binning,
                                   const SampleDataFrameMap& samples,
                                   const ROOT::RDF::TH1DModel& histogram_model,
                                   VariableHistogramFutures& variable_futures) override {
        const auto histogram_stratifier = createStratifier(variable_binning);
        
        for (const auto& [sample_identifier, sample_data] : samples) {
            if (sample_data.first != SampleType::kData) {
                const auto detector_variations = histogram_stratifier->bookDetectorVariations(
                    sample_data.second, variable_binning, histogram_model
                );
                
                mergeSampleSystematics(variable_futures.systematic_variations, detector_variations);
            }
        }
    }
private:
    std::unique_ptr<IHistogramStratifier> createStratifier(const BinDefinition& variable_binning) const {
        return StratifierFactory::create(
            variable_binning.stratification_key.str(),
            stratification_registry_
        );
    }
    
    static ROOT::RDF::RResultPtr<TH1D> combineHistogramFutures(
        const std::vector<ROOT::RDF::RResultPtr<TH1D>>& histogram_futures
    ) {
        if (histogram_futures.empty()) return {};
        
        auto combined_result = histogram_futures[0];
        for (size_t i = 1; i < histogram_futures.size(); ++i) {
            combined_result = combined_result + histogram_futures[i];
        }
        return combined_result;
    }
    
    static void mergeSampleSystematics(SystematicFutures& accumulated_systematics,
                                       const SystematicFutures& sample_systematics) {
        for (const auto& [stratum_key, histogram_future] : sample_systematics.nominal) {
            accumulated_systematics.nominal[stratum_key] = histogram_future;
        }
        
        for (const auto& [systematic_name, systematic_variations] : sample_systematics.variations) {
            for (const auto& [variation_name, variation_histograms] : systematic_variations) {
                for (const auto& [stratum_key, histogram_future] : variation_histograms) {
                    accumulated_systematics.variations[systematic_name][variation_name][stratum_key] = histogram_future;
                }
            }
        }
    }
    
    StratificationRegistry& stratification_registry_;
};

}

#endif