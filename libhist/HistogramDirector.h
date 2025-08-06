#ifndef HISTOGRAM_DIRECTOR_H
#define HISTOGRAM_DIRECTOR_H

#include "BinDefinition.h"
#include "IHistogramBuilder.h"
#include "HistogramResult.h"
#include "IBranchAccessor.h"
#include "ROOT/RDataFrame.hxx"
#include <TH1D.h>
#include <vector>

namespace analysis {

class HistogramDirector : public IHistogramBuilder {
public:
    HistogramResult build(const BinDefinition& bin,
                          const SampleDataFrameMap& dfs) final {
        log::info("HistogramDirector", "ENTER build for variable:", bin.getVariable().Data());
        this->prepareStratification(bin, dfs);
        auto model = this->createModel(bin, dfs);
        ROOT::RDF::RResultPtr<TH1D> dataFuture;
        this->bookNominals(bin, dfs, model, dataFuture);
        this->bookVariations(bin, dfs);
        HistogramResult result;
        if (dataFuture.IsReady()) {
            log::info("HistogramDirector", "Setting data histogram");
            result.setDataHist(
                BinnedHistogram(bin,
                          *dataFuture.GetPtr(),
                          "data_hist",
                          "Data")
            );
        }
        this->mergeStrata(bin, dfs, result);
        this->applySystematicCovariances(bin, result);
        this->finaliseResults(bin, result);
        log::info("HistogramDirector", "EXIT build for variable:", bin.getVariable().Data());
        return result;
    }

protected:
    static BinDefinition resolveBinning(const BinDefinition& spec,
                                        const SampleDataFrameMap& dfs,
                                        IBranchAccessor& accessor) {
        log::info("HistogramDirector", "ENTER resolveBinning for:", spec.getVariable().Data());
        log::info("HistogramDirector", "Number of samples:", dfs.size());
        std::vector<double> values;
        for (auto const& [key, pr] : dfs) {
            log::info("HistogramDirector", "  extracting values from sample:", key);
            auto v = accessor.extractValues(
                pr.second,
                spec.getVariable().Data());
            log::info("HistogramDirector", "    extracted", v.size(), "values");
            values.insert(values.end(), v.begin(), v.end());
        }
        if (values.empty())
            log::fatal("HistogramDirector", "Empty data for binning");
        log::info("HistogramDirector", "Total values for binning:", values.size());
        std::vector<double> edges = spec.edges_;
        BinDefinition out(
            edges,
            spec.getVariable().Data(),
            spec.getTexLabel().Data(),
            spec.getSelectionKeys(),
            spec.getName().Data());
        log::info("HistogramDirector", "EXIT resolveBinning, bins:", out.nBins());
        return out;
    }

    virtual void prepareStratification(const BinDefinition& /*bin*/,
                                       const SampleDataFrameMap& /*dfs*/) {}

    virtual TH1D createModel(const BinDefinition& bin,
                             const SampleDataFrameMap& dfs) = 0;

    virtual void bookNominals(const BinDefinition& bin,
                              const SampleDataFrameMap& dfs,
                              const TH1D& model,
                              ROOT::RDF::RResultPtr<TH1D>& dataFuture) = 0;

    virtual void bookVariations(const BinDefinition& bin,
                                const SampleDataFrameMap& dfs) = 0;

    virtual void mergeStrata(const BinDefinition& bin,
                              const SampleDataFrameMap& dfs,
                              HistogramResult& out) = 0;

    virtual void applySystematicCovariances(const BinDefinition& bin,
                                            HistogramResult& out) = 0;

    virtual void finaliseResults(const BinDefinition& /*bin*/,
                                 HistogramResult& /*out*/) {}
};

}

#endif // HISTOGRAM_DIRECTOR_H