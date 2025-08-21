#ifndef HISTOGRAM_BOOKER_H
#define HISTOGRAM_BOOKER_H

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
        var_future.data_hist_ = ana_dataset.dataframe_.Histo1D(hist_model, binning.getVariable().Data(), "nominal_event_weight");
    }

    void bookNominalHists(const BinningDefinition& binning,
                            const AnalysisDataset& ana_dataset,
                            const ROOT::RDF::TH1DModel& hist_model,
                            VariableFuture& var_future) override {
        const auto hist_stratifier = StratifierFactory::create(binning.getStratifierKey(), stratifier_registry_);
        const auto strat_futurs = hist_stratifier->stratifiyHist(ana_dataset.dataframe_, binning, hist_model);

        var_future.strat_hists_ = strat_futurs;
        
        if (!strat_futurs.empty()) {
            auto it = strat_futurs.begin();
            auto tot_future = it->second;
            ++it;

            var_future.total_mc_ = std::accumulate(it, strat_futurs.end(), tot_future,
                [](ROOT::RDF::RResultPtr<TH1D> current_sum, const auto& pair) {
                    return current_sum + pair.second;
                });
        }
    }

    void bookVariationHists(const BinningDefinition& binning,
                            const VariationKey& variation_key,
                            const AnalysisDataset& variation_sample,
                            const ROOT::RDF::TH1DModel& hist_model,
                            VariableFuture& var_future) override {
        auto hist = variation_sample.dataframe.Histo1D(hist_model, binning.getVariable().Data(), "nominal_event_weight");
        var_future.variation_hists_[variation_key] = hist;
    }

private:
    StratifierRegistry& stratifier_registry_;
};

}

#endif