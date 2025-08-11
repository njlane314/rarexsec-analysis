#ifndef IANALYSIS_PLUGIN_H
#define IANALYSIS_PLUGIN_H

#include "AnalysisDefinition.h"
#include "SelectionRegistry.h"
#include "HistogramResult.h"
#include "AnalysisDataLoader.h"
#include <nlohmann/json.hpp>

namespace analysis {

class IAnalysisPlugin {
public:
    using AnalysisRegionMap = std::map<RegionKey, RegionAnalysis>;

    virtual ~IAnalysisPlugin() = default;

    virtual void onInitialisation(AnalysisDefinition& def,
                              const SelectionRegistry& sel_reg) = 0;

    virtual void onPreSampleProcessing(const RegionKey& region_key,
                                        const SampleKey& sample_key,
                                        const RegionConfig& region) = 0;

    virtual void onPostSampleProcessing(const RegionKey& region_key,
                                        const SampleKey& sample_key,
                                        const AnalysisRegionMap& results) = 0;

    virtual void onFinalisation(const AnalysisRegionMap& results) = 0;
};

}
#endif // IANALYSIS_PLUGIN_H