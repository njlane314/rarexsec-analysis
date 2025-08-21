#ifndef DATAFRAME_HISTOGRAM_BUILDER_H
#define DATAFRAME_HISTOGRAM_BUILDER_H

#include "HistogramBookerDirector.h"
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
#include "AnalysisTypes.h"

namespace analysis {

class HistogramBooker : public HistogramBookerDirector {
public:
    HistogramBooker(StratificationRegistry& strat_reg)
      : stratification_registry_(strat_reg)
    {}

protected:
    void bookDataHists(const BinningDefinition& binning,
                            const AnalysisDataset& ana_dataset,
                            const ROOT::RDF::TH1DModel& hist_model,
                            VariableFuture& var_future) override {
        var_future.data_future = ana_dataset.dataframe.Histo1D(hist_model, binning.getVariable().Data(), "nominal_event_weight");
    }

    void bookNominalHists(const BinningDefinition& binning,
                            const AnalysisDataset& mc_sample,
                            const ROOT::RDF::TH1DModel& hist_model,
                            VariableFuture& var_future) override {
        const auto histogram_stratifier = this->createStratifier(binning);
        const auto channel_histogram_futures = histogram_stratifier->bookNominalHists(
            mc_sample.dataframe, binning, hist_model
        );

        std::vector<ROOT::RDF::RResultPtr<TH1D>> monte_carlo_histogram_futures;
        for (const auto& [channel_identifier, histogram_future] : channel_histogram_futures) {
            var_future.stratified_mc_futures[channel_identifier] = histogram_future;
            monte_carlo_histogram_futures.emplace_back(histogram_future);
        }

        if (!monte_carlo_histogram_futures.empty()) {
            var_future.total_mc_future = combineHistogramFutures(monte_carlo_histogram_futures);
        }
    }

    void bookVariationHists(const BinningDefinition& binning,
                            const AnalysisDataset& variation_sample,
                            const ROOT::RDF::TH1DModel& hist_model,
                            VariableFuture& var_future) override {
        auto hist = variation_sample.dataframe.Histo1D(hist_model, binning.getVariable().Data(), "nominal_event_weight");
        var_future.detector_variation_futures[variation_sample.role] = hist;
    }

private:
    std::unique_ptr<IHistogramStratifier> createStratifier(const BinningDefinition& binning) const {
        return StratifierFactory::create(
            binning.getStratifierKey(),
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