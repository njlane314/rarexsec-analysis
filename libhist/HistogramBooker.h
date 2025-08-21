#ifndef DATAFRAME_HISTOGRAM_BUILDER_H
#define DATAFRAME_HISTOGRAM_BUILDER_H

#include "HistogramBookerDirector.h"
#include "StratifierFactory.h"
#include "StratifierRegistry.h"
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
    HistogramBooker(StratifierRegistry& strat_reg)
      : stratifier_registry_(strat_reg)
    {}

protected:
    void bookDataHists(const BinningDefinition& binning,
                            const AnalysisDataset& ana_dataset,
                            const ROOT::RDF::TH1DModel& hist_model,
                            VariableFuture& var_future) override {
        var_future.data_future = ana_dataset.dataframe_.Histo1D(hist_model, binning.getVariable().Data(), "nominal_event_weight");
    }

    void bookNominalHists(const BinningDefinition& binning,
                            const AnalysisDataset& ana_dataset,
                            const ROOT::RDF::TH1DModel& hist_model,
                            VariableFuture& var_future) override {
        const auto hist_stratifier = this->createStratifier(binning);
        const auto strat_futurs = hist_stratifier->stratifiyHist(ana_dataset.dataframe_, binning, hist_model);

        std::vector<ROOT::RDF::RResultPtr<TH1D>> monte_carlo_histogram_futures;
        for (const auto& [stratum_key, hist_futur] : strat_futurs) {
            var_future.stratified_mc_futures[stratum_key] = hist_futur;
            monte_carlo_histogram_futures.emplace_back(hist_futur);
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
            stratifier_registry_
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

    StratifierRegistry& stratifier_registry_;
};

}

#endif