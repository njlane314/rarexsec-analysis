#ifndef MC_PROCESSOR_H
#define MC_PROCESSOR_H

#include "ISampleProcessor.h"
#include "BinnedHistogram.h"
#include "Logger.h"

namespace analysis {

class MonteCarloProcessor : public ISampleProcessor {
public:
    explicit MonteCarloProcessor(const SampleEnsemble& ensemble)
      : nominal_dataset_(ensemble.nominal_),
        variation_datasets_(ensemble.variations_) {}

    void book(IHistogramBooker& booker,
              const BinningDefinition& binning,
              const ROOT::RDF::TH1DModel& model) override {

        nominal_futures_ = booker.bookStratifiedHists(binning, nominal_dataset_, model);

        if (!variation_datasets_.empty()) {
            for (const auto& [var_key, dataset] : variation_datasets_) {
                variation_futures_[var_key] = booker.bookNominalHist(binning, dataset, model);
            }
        }
    }

    void contributeTo(VariableResult& result) override {
        BinnedHistogram sample_nominal_total;
        std::map<StratumKey, BinnedHistogram> sample_nominal_stratified;
        for (auto& [stratum_key, future] : nominal_futures_) {
            if (future.IsValid()) {
                auto hist = BinnedHistogram::createFromTH1D(result.binning_, *future.GetPtr());
                sample_nominal_stratified[stratum_key] = hist;
                sample_nominal_total += hist;
            }
        }

        for(const auto& [stratum_key, hist] : sample_nominal_stratified) {
            result.strat_hists_[stratum_key] += hist;
        }
        result.total_mc_hist_ += sample_nominal_total;

        if (!variation_datasets_.empty()) {
            std::map<SampleVariation, BinnedHistogram> variation_hists;
            for (auto& [var_key, future] : variation_futures_) {
                if (future.IsValid()) {
                    variation_hists[var_key] = BinnedHistogram::createFromTH1D(result.binning_, *future.GetPtr());
                }
            }

            auto it_cv = variation_hists.find(SampleVariation::kCV);
            if (it_cv == variation_hists.end()) {
                log::warn("MonteCarloProcessor", "Detector variation CV histogram not found. Cannot compute transfer ratios.");
                return;
            }
            const auto& h_det_cv = it_cv->second;

            for (const auto& [var_key, h_det_k] : variation_hists) {
                if (var_key == SampleVariation::kCV) continue; 

                SystematicKey syst_key(variationToKey(var_key));

                auto transfer_ratio = h_det_k / h_det_cv;
                result.transfer_ratio_hists_[syst_key] = transfer_ratio;

                auto h_proj_k = transfer_ratio * result.total_mc_hist_;
                result.variation_hists_[syst_key] = h_proj_k;

                result.delta_hists_[syst_key] = h_proj_k - result.total_mc_hist_;
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