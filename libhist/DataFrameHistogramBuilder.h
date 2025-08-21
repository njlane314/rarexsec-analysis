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
            if (std::get<0>(sample_data) == SampleOrigin::kData) {
                data_histogram_futures.emplace_back(
                    std::get<2>(sample_data).Histo1D(histogram_model, variable_binning.getVariable().Data())
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
            if (std::get<0>(sample_data) != SampleOrigin::kData && std::get<1>(sample_data) == AnalysisRole::kNominal) {
                const auto channel_histogram_futures = histogram_stratifier->bookNominalHistograms(
                    std::get<2>(sample_data), variable_binning, histogram_model
                );
                
                for (const auto& [channel_identifier, histogram_future] : channel_histogram_futures) {
                    variable_futures.stratified_mc_futures[channel_identifier] = histogram_future;
                    monte_carlo_histogram_futures.emplace_back(histogram_future);
                }
            }
        }
        
        if (!monte_carlo_histogram_futures.empty()) {
            variable_futures.total_mc_future = combineHistogramFutures(monte_carlo_histogram_futures);
        }
    }

    void buildDetectorVariationHistograms(const BinDefinition& variable_binning,
                                          const SampleDataFrameMap& samples,
                                          const ROOT::RDF::TH1DModel& histogram_model,
                                          VariableHistogramFutures& variable_futures) override {
        for (const auto& [sample_identifier, sample_data] : samples) {
            if (std::get<1>(sample_data) == AnalysisRole::kSystematicVariation) {
                auto hist = std::get<2>(sample_data).Histo1D(histogram_model, variable_binning.getVariable().Data(), "central_value_weight");
                variable_futures.detector_variation_futures[sample_identifier.str()] = hist;
            }
        }
    }

private:
    std::unique_ptr<IHistogramStratifier> createStratifier(const BinDefinition& variable_binning) const {
        return StratifierFactory::create(
            variable_binning.getStratifierKey().str(),
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
    
    StratificationRegistry& stratification_registry_;
};

}

#endif