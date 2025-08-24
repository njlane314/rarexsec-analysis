#ifndef IANALYSIS_PLUGIN_H
#define IANALYSIS_PLUGIN_H

#include "AnalysisTypes.h"
#include "AnalysisDefinition.h"
#include "SelectionRegistry.h"
#include "AnalysisDataLoader.h"
#include <nlohmann/json.hpp>
#include "RunConfig.h"

namespace analysis {

class IAnalysisPlugin {
public:
    virtual ~IAnalysisPlugin() = default;

    virtual void onInitialisation(
                            AnalysisDefinition& def,
                            const SelectionRegistry& sel_reg) = 0;

    virtual void onPreSampleProcessing(
                            const SampleKey& sample_key,
                            const RegionKey& region_key,
                            const RunConfig& region) = 0;

    virtual void onPostSampleProcessing(
                            const SampleKey& sample_key,
                            const RegionKey& region_key,
                            const AnalysisRegionMap& results) = 0;

    virtual void onFinalisation(const AnalysisRegionMap& results) = 0;
};

}
#endif