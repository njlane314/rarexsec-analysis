#ifndef MC_PROCESSOR_H
#define MC_PROCESSOR_H

#include "ISampleProcessor.h"
#include "BinnedHistogram.h"
#include "Logger.h"

namespace analysis {

class MonteCarloProcessor : public ISampleProcessor {
public:
    explicit MonteCarloProcessor(const SampleKey& key, const SampleEnsemble& ensemble)
      : sample_key_(key),
        nominal_dataset_(ensemble.nominal_),
        variation_datasets_(ensemble.variations_) {}

    void book(IHistogramBooker& booker,
              const BinningDefinition& binning,
              const ROOT::RDF::TH1DModel& model) override {
        analysis::log::info("MonteCarloProcessor::book", "Beginning stratification...");
        nominal_futures_ = booker.bookStratifiedHists(binning, nominal_dataset_, model);
        
        analysis::log::info("MonteCarloProcessor::book", "Booking nominals...");
        if (!variation_datasets_.empty()) {
            for (const auto& [var_key, dataset] : variation_datasets_) {
                variation_futures_[var_key] = booker.bookNominalHist(binning, dataset, model);
            }
        }
    }

    void contributeTo(VariableResult& result) override {
        for (auto& [stratum_key, future] : nominal_futures_) {
            if (future.GetPtr()) {
                auto hist = BinnedHistogram::createFromTH1D(result.binning_, *future.GetPtr());
                ChannelKey channel_key{stratum_key.str()};
                result.strat_hists_[channel_key] = result.strat_hists_[channel_key] + hist;
                result.total_mc_hist_ = result.total_mc_hist_ + hist;
            }
        }

        for (auto& [var_key, future] : variation_futures_) {
            if (future.GetPtr()) {
                result.raw_detvar_hists_[sample_key_][var_key] =
                    BinnedHistogram::createFromTH1D(result.binning_, *future.GetPtr());
            }
        }
    }

private:
    SampleKey sample_key_;
    const AnalysisDataset& nominal_dataset_;
    const std::map<SampleVariation, AnalysisDataset>& variation_datasets_;

    std::unordered_map<StratumKey, ROOT::RDF::RResultPtr<TH1D>> nominal_futures_;
    std::map<SampleVariation, ROOT::RDF::RResultPtr<TH1D>> variation_futures_;
};

}

#endif