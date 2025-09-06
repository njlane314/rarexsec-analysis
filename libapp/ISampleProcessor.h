#ifndef ISAMPLE_PROCESSOR_H
#define ISAMPLE_PROCESSOR_H

#include "AnalysisTypes.h"
#include "HistogramBooker.h"

namespace analysis {

class ISampleProcessor {
  public:
    virtual ~ISampleProcessor() = default;

    virtual void book(HistogramBooker &booker, const BinningDefinition &binning,
                      const ROOT::RDF::TH1DModel &model) = 0;

    virtual void contributeTo(VariableResult &result) = 0;
};

}

#endif
