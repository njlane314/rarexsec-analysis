#ifndef MC_PROCESSOR_H
#define MC_PROCESSOR_H

#include "ISampleProcessor.h"

namespace analysis {

class MonteCarloProcessor : public ISampleProcessor {
public:
    explicit MonteCarloProcessor(const SampleEnsemble& ensemble)
      : nominal_dataset_(ensemble.nominal_),
        variation_datasets_(ensemble.variations_) {}

    void book(IHistogramBooker& booker,
              const BinningDefinition& binning,
              const ROOT::RDF::TH1DModel& model) override {
        
        nominal_futures_ = booker.bookStratifiedHistograms(binning, nominal_dataset_, model);

        if (!variation_datasets_.empty()) {
            for (const auto& [var_key, dataset] : variation_datasets_) {
                variation_futures_[var_key] = booker.bookUnstratifiedHistogram(binning, dataset, model);
            }
        }
    }

    void contributeTo(VariableResult& final_result,
                      SystematicsProcessor& systematics_processor) override {
        
        BinnedHistogram sample_nominal_total;
        std::map<StratumKey, BinnedHistogram> sample_nominal_stratified;
        for (auto& [stratum_key, future] : nominal_futures_) {
            if (future.IsValid()) {
                auto hist = BinnedHistogram::createFromTH1D(final_result.binning_, *future.GetPtr());
                sample_nominal_stratified[stratum_key] = hist;
                sample_nominal_total += hist;
            }
        }

        for(const auto& [stratum_key, hist] : sample_nominal_stratified) {
            final_result.strat_hists_[stratum_key] += hist;
        }
        final_result.total_mc_hist_ += sample_nominal_total;

        if (!variation_futures_.empty() && variation_futures_.count(SampleVariation::kCV)) {
            BinnedHistogram cv_hist = BinnedHistogram::createFromTH1D(final_result.binning_, *variation_futures_.at(SampleVariation::kCV).GetPtr());

            for (auto& [var_key, future] : variation_futures_) {
                if (var_key == SampleVariation::kCV) continue;
                
                BinnedHistogram varied_hist = BinnedHistogram::createFromTH1D(final_result.binning_, *future.GetPtr());
                BinnedHistogram delta = varied_hist - cv_hist;

                // Here, you would pass the delta and other info to the systematics_processor
                // to accumulate for the final covariance matrix calculation.
                // For example:
                // systematics_processor.accumulateDelta(variationToKey(var_key), delta);
            }
        }
    }

private:
    const AnalysisDataset& nominal_dataset_;
    const std::map<SampleVariation, AnalysisDataset>& variation_datasets_;
    
    std::unordered_map<StratumKey, ROOT::RDF::RResultPtr<TH1D>> nominal_futures_;
    std::map<SampleVariation, ROOT::RDF::RResultPtr<TH1D>> variation_futures_;
};

}

#endif