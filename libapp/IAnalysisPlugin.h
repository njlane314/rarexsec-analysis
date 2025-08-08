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
    using AnalysisResultMap = std::map<std::string, HistogramResult>;
    virtual ~IAnalysisPlugin() = default;

    virtual void onInitialisation(AnalysisDefinition& def,
                              const SelectionRegistry& sel_reg) = 0;

    virtual void onPreSampleProcessing(const std::string& region_key,
                                        const RegionConfig& region,
                                        const std::string& sample_key) = 0;

    virtual void onPostSampleProcessing(const std::string& region_key,
                                        const std::string& sample_key,
                                        const AnalysisResultMap& results) = 0;

    virtual void onFinalisation(const AnalysisResultMap& all_results) = 0;
};

}
#endif // IANALYSIS_PLUGIN_H