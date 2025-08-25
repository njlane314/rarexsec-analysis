#ifndef ISAMPLE_PROCESSOR_H
#define ISAMPLE_PROCESSOR_H

#include "AnalysisTypes.h"
#include "IHistogramBooker.h"

namespace analysis {

class ISampleProcessor {
  public:
    virtual ~ISampleProcessor() = default;

    virtual void book(IHistogramBooker &booker,
                      const BinningDefinition &binning,
                      const ROOT::RDF::TH1DModel &model) = 0;

    virtual void contributeTo(VariableResult &result) = 0;
};

} // namespace analysis

#endif
