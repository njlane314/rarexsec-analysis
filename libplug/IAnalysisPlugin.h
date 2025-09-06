#ifndef IANALYSIS_PLUGIN_H
#define IANALYSIS_PLUGIN_H

#include <nlohmann/json.hpp>

#include "AnalysisDataLoader.h"
#include "AnalysisDefinition.h"
#include "AnalysisResult.h"
#include "RegionAnalysis.h"
#include "RunConfig.h"
#include "SelectionRegistry.h"

namespace analysis {

class IAnalysisPlugin {
  public:
    virtual ~IAnalysisPlugin() = default;

    virtual void onInitialisation(AnalysisDefinition &def, const SelectionRegistry &sel_reg) = 0;

    virtual void onPreSampleProcessing(const SampleKey &sample_key, const RegionKey &region_key,
                                       const RunConfig &run_config) = 0;

    virtual void onPostSampleProcessing(const SampleKey &sample_key, const RegionKey &region_key,
                                        const RegionAnalysisMap &results) = 0;

    virtual void onFinalisation(const AnalysisResult &results) = 0;
};

}
#endif
