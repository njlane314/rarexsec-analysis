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
                      const TH1D& model,
                      ROOT::RDF::RResultPtr<TH1D>& dataFuture) override {
        for (auto const& [key, pr] : dfs) {
            auto& df = pr.second;
            if (pr.first == SampleType::kData) {
                dataFuture = df.Histo1D(model, bin.getVariable().Data());
            } else {
                histogram_futures_[key] =
                    stratifier_->bookHistograms(df, bin, model);
            }
        }
    }

    void bookVariations(const BinDefinition& bin,
                        const SampleDataFrameMap& dfs) override {
        for (auto const& [key, pr] : dfs) {
            if (pr.first == SampleType::kData) continue;
            stratifier_->bookSystematicsHistograms(
                pr.second,
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
        for (auto& [name, futs] : histogram_futures_) {
            auto hist_map = stratifier_->collectHistograms(futs, bin);
            for (auto const& [sname, h] : hist_map) {
                out.addChannel(sname, h);
                total = total + h;
            }
        }
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
    std::map<std::string, std::map<int, ROOT::RDF::RResultPtr<TH1D>>>   histogram_futures_;
};

}

#endif // DATAFRAME_HISTOGRAM_BUILDER_H