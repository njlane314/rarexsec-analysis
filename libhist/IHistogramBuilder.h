#ifndef IHISTOGRAM_BUILDER_H
#define IHISTOGRAM_BUILDER_H

#include "BinDefinition.h"
#include "RegionAnalysis.h"
#include "SampleTypes.h"
#include "Keys.h"
#include <ROOT/RDataFrame.hxx>
#include <map>
#include <string>
#include <vector>

namespace analysis {

class IHistogramBuilder {
public:
    using SampleDataFrameMap = std::map<std::string, std::pair<SampleType, ROOT::RDF::RNode>>;
    using HistogramFuturePtr = ROOT::RDF::RResultPtr<TH1D>;
    using StratumToHistogramMap = std::unordered_map<int, HistogramFuturePtr>;
    using SampleToStratumMap = std::unordered_map<std::string, StratumToHistogramMap>;

    virtual ~IHistogramBuilder() = default;

    virtual RegionAnalysis buildRegionAnalysis(
        const RegionKey& region_key,
        const std::vector<std::pair<VariableKey, BinDefinition>>& variable_definitions,
        const SampleDataFrameMap& sample_dataframes
    ) = 0;
    
    MaterialisedHistograms build(const BinDefinition& variable_binning,
                                 const SampleDataFrameMap& sample_dataframes) {
        const VariableKey variable{variable_binning.getName().Data()};
        const RegionKey default_region{"default_region"};
        
        auto region_analysis = buildRegionAnalysis(
            default_region,
            {{variable, variable_binning}},
            sample_dataframes
        );
    }
};

}

#endif // IHISTOGRAM_BUILDER_H
