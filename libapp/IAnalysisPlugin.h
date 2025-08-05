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
    virtual ~IAnalysisPlugin() = default;

    virtual void onInitialise(const AnalysisDefinition& def,
                              const SelectionRegistry& selReg) = 0;

    virtual void onPreRegion(const std::string& regionKey,
                             const RegionConfig& region,
                             const std::string& sampleKey) = 0;

    virtual void onPostRegion(const std::string& regionKey,
                              const std::string& sampleKey,
                              const HistogramResult& results) = 0;

    virtual void onFinalise(const HistogramResult& allResults) = 0;
};

} 
#endif // IANALYSIS_PLUGIN_H