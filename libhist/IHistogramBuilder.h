#ifndef IHISTOGRAM_BUILDER_H
#define IHISTOGRAM_BUILDER_H

#include "BinDefinition.h"
#include "HistogramResult.h"
#include "SampleTypes.h"
#include <ROOT/RDataFrame.hxx>
#include <map>
#include <string>

namespace analysis {

class IHistogramBuilder {
public:
    using SampleDataFrameMap = std::map<std::string, std::pair<SampleType, ROOT::RDF::RNode>>;

    virtual ~IHistogramBuilder() = default;

    virtual HistogramResult build(const BinDefinition&                bins,
                                  const SampleDataFrameMap&           dfs) = 0;
};

}

#endif // IHISTOGRAM_BUILDER_H
