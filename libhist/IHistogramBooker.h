#ifndef IHISTOGRAM_BOOKER_H
#define IHISTOGRAM_BOOKER_H

#include "BinDefinition.h"
#include "RegionAnalysis.h"
#include "AnalysisTypes.h"
#include "Keys.h"
#include <map>
#include <string>
#include <vector>

namespace analysis {

class IHistogramBooker {
public:
    virtual ~IHistogramBooker() = default;

    virtual std::map<VariableKey, VariableFutures> bookHistograms(
        const std::vector<std::pair<VariableKey, BinDefinition>>& variable_definitions,
        const SampleEnsembleMap& sample_ensembles
    ) = 0;
};

}

#endif // IHISTOGRAM_BOOKER_H
