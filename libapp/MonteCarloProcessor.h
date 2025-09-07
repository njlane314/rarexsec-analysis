#ifndef MC_PROCESSOR_H
#define MC_PROCESSOR_H

#include "AnalysisLogger.h"
#include "BinnedHistogram.h"
#include "ISampleProcessor.h"
#include <unordered_map>

namespace analysis {

class MonteCarloProcessor : public ISampleProcessor {
  public:
    explicit MonteCarloProcessor(const SampleKey &key, SampleDatasetGroup ensemble)
        : sample_key_(key), nominal_dataset_(std::move(ensemble.nominal_)),
          variation_datasets_(std::move(ensemble.variations_)) {}

    void book(HistogramFactory &factory, const BinningDefinition &binning, const ROOT::RDF::TH1DModel &model) override {
        analysis::log::info("MonteCarloProcessor::book", "Beginning stratification...");
        nominal_futures_ = factory.bookStratifiedHists(binning, nominal_dataset_, model);

        analysis::log::info("MonteCarloProcessor::book", "Booking nominals...");
        if (!variation_datasets_.empty()) {
            for (auto &[var_key, dataset] : variation_datasets_) {
                variation_futures_[var_key] = factory.bookNominalHist(binning, dataset, model);
            }
        }
    }

    void contributeTo(VariableResult &result) override {
        log::info("MonteCarloProcessor::contributeTo", "Contributing histograms from sample:", sample_key_.str());
        for (auto &[stratum_key, future] : nominal_futures_) {
            if (future.GetPtr()) {
                auto hist = BinnedHistogram::createFromTH1D(result.binning_, *future.GetPtr());
                ChannelKey channel_key{stratum_key.str()};
                result.strat_hists_[channel_key] = result.strat_hists_[channel_key] + hist;
                result.total_mc_hist_ = result.total_mc_hist_ + hist;
            }
        }

        for (auto &[var_key, future] : variation_futures_) {
            if (future.GetPtr()) {
                result.raw_detvar_hists_[sample_key_][var_key] =
                    BinnedHistogram::createFromTH1D(result.binning_, *future.GetPtr());
            }
        }
    }

  private:
    SampleKey sample_key_;
    SampleDataset nominal_dataset_;
    std::unordered_map<SampleVariation, SampleDataset> variation_datasets_;

    std::unordered_map<StratumKey, ROOT::RDF::RResultPtr<TH1D>> nominal_futures_;
    std::unordered_map<SampleVariation, ROOT::RDF::RResultPtr<TH1D>> variation_futures_;
};

}

#endif
