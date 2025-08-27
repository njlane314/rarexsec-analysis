#ifndef IHISTOGRAM_BOOKER_H
#define IHISTOGRAM_BOOKER_H

#include "AnalysisTypes.h"
#include "BinningDefinition.h"

namespace analysis {

class IHistogramBooker {
  public:
    virtual ~IHistogramBooker() = default;

    virtual ROOT::RDF::RResultPtr<TH1D>
    bookNominalHist(const BinningDefinition &binning,
                    const SampleDataset &dataset,
                    const ROOT::RDF::TH1DModel &model) = 0;

    virtual std::unordered_map<StratumKey, ROOT::RDF::RResultPtr<TH1D>>
    bookStratifiedHists(const BinningDefinition &binning,
                        const SampleDataset &dataset,
                        const ROOT::RDF::TH1DModel &model) = 0;
};

}

#endif
