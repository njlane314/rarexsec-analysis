#ifndef HISTOGRAM_DIRECTOR_H
#define HISTOGRAM_DIRECTOR_H

#include "BinDefinition.h"
#include "IHistogramBuilder.h"
#include "HistogramResult.h"
#include "ROOT/RDataFrame.hxx"
#include <TH1D.h>
#include <vector>
#include <iterator>

namespace analysis {

class HistogramDirector : public IHistogramBuilder {
public:
    HistogramResult build(const BinDefinition& bin,
                          const SampleDataFrameMap& dfs) final {
        this->prepareStratification(bin, dfs);
        auto model = createModel(bin, dfs);
        ROOT::RDF::RResultPtr<TH1D> dataFuture;
        this->bookNominals(bin, dfs, model, dataFuture);
        this->bookVariations(bin, dfs);
        HistogramResult result;
        if (dataFuture && dataFuture.IsReady()) {
            // Use the static factory function here
            result.setDataHist(BinnedHistogram::createFromTH1D(bin, *dataFuture.GetPtr(), "data_hist", "Data"));
        }
        this->mergeStrata(bin, dfs, result);
        this->applySystematicCovariances(bin, result);
        this->finaliseResults(bin, result);
        return result;
    }

protected:
    static ROOT::RDF::TH1DModel makeModel(const BinDefinition& bin,
                                          const SampleDataFrameMap& dfs) {
        auto df0 = dfs.begin()->second.second;
        auto loFut = df0.Min(bin.getVariable().Data());
        auto hiFut = df0.Max(bin.getVariable().Data());
        double lo = *loFut;
        double hi = *hiFut;
        int N = static_cast<int>(bin.nBins());
        std::vector<double> edges(N + 1);
        for (int i = 0; i <= N; ++i) edges[i] = lo + (hi - lo) * i / N;
        return ROOT::RDF::TH1DModel(
            bin.getName().Data(),
            bin.getName().Data(),
            static_cast<int>(edges.size() - 1),
            edges.data()
        );
    }

    ROOT::RDF::TH1DModel createModel(const BinDefinition& bin,
                                     const SampleDataFrameMap& dfs) {
        return makeModel(bin, dfs);
    }

    virtual void prepareStratification(const BinDefinition&,
                                       const SampleDataFrameMap&) {}

    virtual void bookNominals(const BinDefinition&,
                              const SampleDataFrameMap&,
                              const ROOT::RDF::TH1DModel&,
                              ROOT::RDF::RResultPtr<TH1D>&) = 0;

    virtual void bookVariations(const BinDefinition&,
                                const SampleDataFrameMap&) = 0;

    virtual void mergeStrata(const BinDefinition&,
                             const SampleDataFrameMap&,
                             HistogramResult&) = 0;

    virtual void applySystematicCovariances(const BinDefinition&,
                                            HistogramResult&) = 0;

    virtual void finaliseResults(const BinDefinition&,
                                 HistogramResult&) {}
};

}

#endif