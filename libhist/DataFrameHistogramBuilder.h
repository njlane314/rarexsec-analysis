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
#include <string>
#include <vector>
#include "IBranchAccessor.h"

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
                               const SampleDataFrameMap& /*dfs*/) override
    {
        log::info("DataFrameHistogramBuilder", "ENTER prepareStratification for:", bin.stratification_key);
        stratifier_ = StratifierFactory::create(bin.stratification_key, stratifier_registry_);
        log::info("DataFrameHistogramBuilder", "Stratifier created for:", bin.stratification_key);
        branch_accessor_ = BranchAccessorFactory::create(stratifier_->getRequiredBranchType());
        log::info("DataFrameHistogramBuilder", "BranchAccessor created");
        log::info("DataFrameHistogramBuilder", "EXIT prepareStratification for:", bin.stratification_key);
    }

    TH1D createModel(const BinDefinition& bin,
                     const SampleDataFrameMap& dfs) override
    {
        log::info("DataFrameHistogramBuilder", "ENTER createModel for variable:", bin.getVariable().Data());
        log::info("DataFrameHistogramBuilder", "About to call resolveBinning");
        auto rb = resolveBinning(bin, dfs, *branch_accessor_);
        log::info("DataFrameHistogramBuilder", "resolveBinning returned name=", rb.getName().Data(),
                                        " nBins=", rb.nBins(),
                                        " edges=[", rb.edges_.front(), ",... ,", rb.edges_.back(), "]");
        TH1D hist(rb.getName().Data(), rb.getName().Data(), rb.nBins(), rb.edges_.data());
        log::info("DataFrameHistogramBuilder", "Model created with", rb.nBins(), "bins");
        log::info("DataFrameHistogramBuilder", "EXIT createModel for variable:", bin.getVariable().Data());
        return hist;
    }

    void bookNominals(const BinDefinition& bin,
                      const SampleDataFrameMap& dfs,
                      const TH1D& model,
                      ROOT::RDF::RResultPtr<TH1D>& data_future) override
    {
        log::info("DataFrameHistogramBuilder", "ENTER bookNominals for:", bin.getVariable().Data());
        for (auto const& [key, pr] : dfs) {
            ROOT::RDF::RNode df = pr.second;
            log::info("DataFrameHistogramBuilder", "  booking nominal for sample:", key);
            auto type = pr.first;
            if (type == SampleType::kData) {
                log::info("DataFrameHistogramBuilder", "    sample is data, scheduling Histo1D");
                data_future = df.Histo1D(ROOT::RDF::TH1DModel(model), bin.getVariable().Data());
            } else {
                log::info("DataFrameHistogramBuilder", "    sample is MC, scheduling stratified histograms");
                try {
                    log::info("DataFrameHistogramBuilder", "    about to bookHistograms for sample:", key);
                    histogram_futures_[key] = stratifier_->bookHistograms(df, bin, model);
                    log::info("DataFrameHistogramBuilder", "    bookHistograms succeeded for sample:", key);
                } catch (const std::exception& e) {
                    log::fatal("DataFrameHistogramBuilder",
                               "    EXCEPTION in bookHistograms for sample:", key,
                               "  what():", e.what());
                } catch (...) {
                    log::fatal("DataFrameHistogramBuilder",
                               "    UNKNOWN EXCEPTION in bookHistograms for sample:", key);
                }
            }
        }
        log::info("DataFrameHistogramBuilder", "EXIT bookNominals for:", bin.getVariable().Data());
    }

    void bookVariations(const BinDefinition& bin,
                        const SampleDataFrameMap& dfs) override
    {
        log::info("DataFrameHistogramBuilder", "ENTER bookVariations for:", bin.getVariable().Data());
        for (auto const& [key, pr] : dfs) {
            if (pr.first == SampleType::kData) continue;
            log::info("DataFrameHistogramBuilder", "  booking variations for sample:", key);
            stratifier_->bookSystematicsHistograms(
                pr.second,
                bin,
                [&](int idx,
                    ROOT::RDF::RNode,
                    const BinDefinition&,
                    const std::string&,
                    const std::string&,
                    const std::string&)
                {
                    log::info("DataFrameHistogramBuilder", "    booking variation idx:", idx);
                    systematics_processor_.bookVariations(idx);
                }
            );
        }
        log::info("DataFrameHistogramBuilder", "EXIT bookVariations for:", bin.getVariable().Data());
    }

    void mergeStrata(const BinDefinition& bin,
                     const SampleDataFrameMap& /*dfs*/,
                     HistogramResult& out) override
    {
        log::info("DataFrameHistogramBuilder", "ENTER mergeStrata for:", bin.getVariable().Data());
        for (auto& [name, futs] : histogram_futures_) {
            log::info("DataFrameHistogramBuilder", "  collecting histograms for stratum:", name);
            auto hist_map = stratifier_->collectHistograms(futs, bin);
            for (auto const& [sname, h] : hist_map) {
                log::info("DataFrameHistogramBuilder", "    adding channel:", sname);
                out.addChannel(sname, h);
                total_nominal_histogram_ = total_nominal_histogram_ + h;
            }
        }
        out.setTotalHist(total_nominal_histogram_);
        log::info("DataFrameHistogramBuilder", "EXIT mergeStrata for:", bin.getVariable().Data());
    }

    void applySystematicCovariances(const BinDefinition& bin,
                                    HistogramResult& out) override
    {
        log::info("DataFrameHistogramBuilder", "ENTER applySystematicCovariances for:", bin.getVariable().Data());
        stratifier_->applySystematics(out, bin, systematics_processor_);
        log::info("DataFrameHistogramBuilder", "EXIT applySystematicCovariances for:", bin.getVariable().Data());
    }

private:
    using TH1DFutures = std::map<int, ROOT::RDF::RResultPtr<TH1D>>;

    SystematicsProcessor&                    systematics_processor_;
    StratificationRegistry&                  stratifier_registry_;
    std::unique_ptr<IHistogramStratifier>    stratifier_;
    std::unique_ptr<IBranchAccessor>         branch_accessor_;
    std::map<std::string, TH1DFutures>       histogram_futures_;
    BinnedHistogram                          total_nominal_histogram_;
};

} // namespace analysis

#endif // DATAFRAME_HISTOGRAM_BUILDER_H
