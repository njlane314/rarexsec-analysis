#ifndef DATAFRAME_HISTOGRAM_BUILDER_H
#define DATAFRAME_HISTOGRAM_BUILDER_H

#include "HistogramDirector.h"
#include "StratifierFactory.h"
#include "StratificationRegistry.h"
#include "SystematicsProcessor.h"
#include "ROOT/RDataFrame.hxx"
#include <TH1D.h>
#include <map>
#include <string>
#include <vector>
#include <unordered_map>
#include "Logger.h"

namespace analysis {

class DataFrameHistogramBuilder : public HistogramDirector {
public:
    DataFrameHistogramBuilder(SystematicsProcessor& sys,
                              StratificationRegistry& strReg)
      : systematics_processor_(sys)
      , stratifier_registry_(strReg)
    {}

protected:
    void prepareStratification(const BinDefinition& bin,
                               const SampleDataFrameMap&) override {
        stratifier_ = StratifierFactory::create(bin.stratification_key,
                                                stratifier_registry_);
    }

    void bookNominals(const BinDefinition& bin,
                      const SampleDataFrameMap& dfs,
                      const ROOT::RDF::TH1DModel& model,
                      ROOT::RDF::RResultPtr<TH1D>& dataFuture) override {
        for (auto const& [key, pr] : dfs) {
            auto df = pr.second;
            if (pr.first == SampleType::kData) {
                dataFuture = df.Histo1D(model, bin.getVariable().Data());
            } else {
                std::vector<double> edges;
                edges.reserve(bin.nBins() + 1);
                const int N = static_cast<int>(bin.nBins());
                if (!bin.edges_.empty()) {
                    edges = bin.edges_;
                } else {
                    edges.resize(N + 1);
                    for (int i = 0; i <= N; ++i) edges[i] = i;
                }
                TH1D thModel(bin.getName().Data(), bin.getName().Data(), static_cast<int>(edges.size() - 1), edges.data());
                
                auto stratified_df = stratifier_->defineStratificationColumns(df, bin);
                histogram_futures_[key] = stratifier_->bookHistograms(stratified_df, bin, thModel);
            }
        }
    }

    void bookVariations(const BinDefinition& bin,
                        const SampleDataFrameMap& dfs) override {
        for (auto const& [key, pr] : dfs) {
            if (pr.first == SampleType::kData) continue;
            
            auto stratified_df = stratifier_->defineStratificationColumns(pr.second, bin);

            stratifier_->bookSystematicsHistograms(
                stratified_df,
                bin,
                [&](int idx,
                    ROOT::RDF::RNode,
                    const BinDefinition&,
                    const std::string&,
                    const std::string&,
                    const std::string&) {
                    systematics_processor_.bookVariations(idx);
                }
            );
        }
    }

    void mergeStrata(const BinDefinition& bin,
                     const SampleDataFrameMap&,
                     HistogramResult& out) override {
        BinnedHistogram total;
        bool first = true;

        log::debug("DataFrameHistogramBuilder::mergeStrata", "Starting merge for variable:", std::string(bin.getName().Data()));
        for (auto& [sample_name, futs] : histogram_futures_) {
            log::debug("DataFrameHistogramBuilder::mergeStrata", "Processing sample:", sample_name);
            if (futs.empty()) {
                log::warn("DataFrameHistogramBuilder::mergeStrata", "No histogram futures found for sample:", sample_name);
                continue;
            }

            auto hist_map = stratifier_->collectHistograms(futs, bin);
            log::debug("DataFrameHistogramBuilder::mergeStrata", "Collected", hist_map.size(), "stratified histograms for sample:", sample_name);

            for (auto const& [stratum_name, h] : hist_map) {
                out.addChannel(stratum_name, h);
                log::debug("DataFrameHistogramBuilder::mergeStrata", "Merging stratum:", stratum_name, "with", h.nBins(), "bins.");
                if (first) {
                    log::debug("DataFrameHistogramBuilder::mergeStrata", "Initializing 'total' histogram with the first valid stratum:", stratum_name);
                    total = h;
                    first = false;
                } else {
                    log::debug("DataFrameHistogramBuilder::mergeStrata", "Adding stratum:", stratum_name, "(", h.nBins(), "bins) to 'total' (", total.nBins(), "bins)");
                    total = total + h;
                }
            }
        }
        if (first) {
            log::warn("DataFrameHistogramBuilder::mergeStrata", "No histograms were merged. The 'total' histogram is uninitialized.");
        }
        log::debug("DataFrameHistogramBuilder::mergeStrata", "Final 'total' histogram has", total.nBins(), "bins.");
        out.setTotalHist(total);
    }

    void applySystematicCovariances(const BinDefinition& bin,
                                    HistogramResult& out) override {
        stratifier_->applySystematics(out, bin, systematics_processor_);
    }

private:
    SystematicsProcessor&                                               systematics_processor_;
    StratificationRegistry&                                             stratifier_registry_;
    std::unique_ptr<IHistogramStratifier>                               stratifier_;
    std::unordered_map<std::string, std::map<int, ROOT::RDF::RResultPtr<TH1D>>>   histogram_futures_;
};

}

#endif