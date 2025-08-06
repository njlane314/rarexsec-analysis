#ifndef DATAFRAME_HISTOGRAM_BUILDER_H
#define DATAFRAME_HISTOGRAM_BUILDER_H

#include "HistogramDirector.h"
#include "StratifierFactory.h"
#include "BranchAccessorFactory.h"
#include "StratificationRegistry.h"
#include "SystematicsProcessor.h"
#include "ROOT/RDataFrame.hxx"
#include <TH1D.h>
#include <TMatrixDSym.h>
#include <map>

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
                               const SampleDataFrameMap& dfs) override
    {
        stratifier_ = StratifierFactory::create(bin.getStratificationKey(), stratifier_registry_);
        branch_accessor_   = BranchAccessorFactory::create(stratifier_->getRequiredBranchType());
    }

    TH1D createModel(const BinDefinition& bin,
                     const SampleDataFrameMap& dfs) override
    {
        auto rb = resolveBinning(bin, dfs, stratifier_->getRequiredBranchType());
        return TH1D(rb.getName().Data(), rb.getName().Data(), rb.numBins(), rb.edges().data());
    }

    void bookNominals(const BinDefinition& bin,
                      const SampleDataFrameMap& dfs,
                      const TH1D& model,
                      ROOT::RDF::RResultPtr<TH1D>& data_future) override
    {
        for (auto const& [key, pr] : dfs) {
            auto type = pr.first;
            auto df   = pr.second;
            if (type == SampleType::kData) {
                data_future = df.Histo1D(model, bin.getVariable().Data());
            } else {
                histogram_futures_[key] = stratifier_->bookHistograms(df, bin, model);
            }
        }
    }

    void bookVariations(const BinDefinition& bin,
                        const SampleDataFrameMap& dfs) override
    {
        auto vars = systematics_processor_.getAssociatedVariations();
        for (auto const& [key, pr] : dfs) {
            if (pr.first == SampleType::kData) continue;
            stratifier_->bookSystematicsHistograms(
                pr.second,
                bin,
                [&](int idx,
                    ROOT::RDF::RNode df,
                    const BinDefinition& tb,
                    const std::string& hname,
                    const std::string& catcol,
                    const std::string& param)
                {
                    systematics_processor_.bookVariations(
                        hname,
                        key,
                        df,
                        vars,
                        tb,
                        bin.getSelectionQuery().Data(),
                        catcol,
                        param
                    );
                }
            );
        }
    }

    void mergeStrata(const BinDefinition& bin,
                     const SampleDataFrameMap& /*dfs*/,
                     HistogramResult& out) override
    {
        for (auto const& [name, futs] : histogram_futures_) {
            auto hist_map = stratifier_->collectHistograms(futs, bin);
            for (auto const& [sname, h] : hist_map) {
                out.addChannel(sname, h);
                total_nominal_histogram_ = total_nominal_histogram_ + h;
            }
        }
        out.setTotalHist(total_nominal_histogram_);
    }

    void applySystematicCovariances(const BinDefinition& bin,
                                    HistogramResult& out) override
    {
        stratifier_->applySystematics(out, bin, systematics_processor_);
    }

private:
    using TH1DFutures = std::map<int, ROOT::RDF::RResultPtr<TH1D>>; 

    SystematicsProcessor&                                       systematics_processor_;
    StratificationRegistry&                                     stratifier_registry_;
    std::unique_ptr<IHistogramStratificationStrategy>           stratifier_;
    std::unique_ptr<IBranchAccessor>                            branch_accessor_;
    std::map<std::string, TH1DFutures>                          histogram_futures_;
    BinnedHistogram                                             total_nominal_histogram_;
};

}

#endif // DATAFRAME_HISTOGRAM_BUILDER_H