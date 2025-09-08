#ifndef MC_PROCESSOR_H
#define MC_PROCESSOR_H

#include "Logger.h"
#include "BinnedHistogram.h"
#include "ISampleProcessor.h"
#include <unordered_map>
#include <mutex>
#include <tbb/parallel_for_each.h>

namespace analysis {

class MonteCarloProcessor : public ISampleProcessor {
  public:
    explicit MonteCarloProcessor(const SampleKey &key, SampleDatasetGroup ensemble)
        : sample_key_(key), nominal_dataset_(std::move(ensemble.nominal_)),
          variation_datasets_(std::move(ensemble.variations_)) {}

    void book(HistogramFactory &factory, const BinningDefinition &binning, const ROOT::RDF::TH1DModel &model) override {
        analysis::log::info("MonteCarloProcessor::book", "Beginning stratification...");
        analysis::log::debug("MonteCarloProcessor::book", "Requested stratifier key:", binning.getStratifierKey().str());
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
        std::mutex mtx;
        tbb::parallel_for_each(
            nominal_futures_.begin(), nominal_futures_.end(),
            [&, this](auto &entry) {
                auto &[stratum_key, future] = entry;
                if (future.GetPtr()) {
                    auto hist =
                        BinnedHistogram::createFromTH1D(result.binning_, *future.GetPtr());
                    ChannelKey channel_key{stratum_key.str()};
                    std::lock_guard<std::mutex> lock(mtx);
                    result.strat_hists_[channel_key] =
                        result.strat_hists_[channel_key] + hist;
                    result.total_mc_hist_ = result.total_mc_hist_ + hist;
                }
            });

        tbb::parallel_for_each(
            variation_futures_.begin(), variation_futures_.end(),
            [&, this](auto &entry) {
                auto &[var_key, future] = entry;
                if (future.GetPtr()) {
                    auto hist =
                        BinnedHistogram::createFromTH1D(result.binning_, *future.GetPtr());
                    std::lock_guard<std::mutex> lock(mtx);
                    result.raw_detvar_hists_[sample_key_][var_key] = hist;
                }
            });
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
