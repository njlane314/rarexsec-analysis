#ifndef ISAMPLE_PROCESSOR_H
#define ISAMPLE_PROCESSOR_H

#include "VariableResult.h"
#include "HistogramFactory.h"
#include "BinningDefinition.h"
#include <ROOT/RDataFrame.hxx>
#include <vector>
#include <cstddef>

namespace analysis {

class ISampleProcessor {
  public:
    virtual ~ISampleProcessor() = default;

    virtual void book(HistogramFactory &factory, const BinningDefinition &binning,
                      const ROOT::RDF::TH1DModel &model) = 0;

    virtual void collectHandles(std::vector<ROOT::RDF::RResultHandle> &handles) = 0;

    virtual VariableResult contribute(const BinningDefinition &binning) = 0;

    virtual std::size_t expectedHandleCount() const = 0;
};

}

#endif
