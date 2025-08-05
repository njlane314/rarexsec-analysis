#ifndef HISTOGRAM_BUILDER_BASE_H
#define HISTOGRAM_BUILDER_BASE_H

#include "BinDefinition.h"
#include "IHistogramBuilder.h"
#include "HistogramResult.h"
#include <ROOT/RDF/RResultPtr.hxx>
#include <TH1D.h>

namespace analysis {

class HistogramDirector : public IHistogramBuilder {
public:
    HistogramResult build(const BinDefinition& bin,
                          const SampleDataFrameMap& dfs) final {
        this->prepareStratification(bin, dfs);
        auto model = this->createModel(bin, dfs);

        ROOT::RDF::RResultPtr<TH1D> dataFuture;
        this->bookNominals(bin, dfs, model, dataFuture);
        this->bookVariations(bin, dfs);

        HistogramResult result;
        if (dataFuture.IsValid()) {
            result.setDataHist(
                Histogram(bin,
                          *dataFuture.GetPtr(),
                          "data_hist",
                          "Data")
            );
        }

        this->mergeStrata(bin, dfs, result);
        this->applySystematicCovariances(bin, result);
        this->finaliseResults(bin, result);
        return result;
    }

protected:
    static BinDefinition resolveBinning(const BinDefinition& spec,
                                        const SampleDataFrameMap& dfs,
                                        IBranchAccessor& accessor) {
        std::vector<double> values;
        for (auto const& [key, pr] : dfs) {
            auto v = accessor.extractValues(
                pr.second,
                spec.getVariable().Data());
            values.insert(values.end(), v.begin(), v.end());
        }

        if (values.empty())
            log::fatal("HistogramDirector", "Empty data for binning");

        auto edges = BinningOptimiser::makeEdges(spec, values);
        return BinDefinition(
            edges,
            spec.getVariable().Data(),
            spec.getTexLabel(),
            spec.getSelectionKeys(),
            spec.getName());
    }

    virtual void prepareStratification(const BinDefinition& bin,
                                       const SampleDataFrameMap& dfs) {}

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

    virtual void finaliseResults(const BinDefinition& bin,
                                 HistogramResult& out) {}
};

}

#endif // HISTOGRAM_BUILDER_BASE_H
