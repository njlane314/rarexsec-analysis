#ifndef IANALYSIS_PLUGIN_H
#define IANALYSIS_PLUGIN_H

#include <nlohmann/json.hpp>

#include <rarexsec/core/AnalysisDefinition.h>
#include <rarexsec/core/AnalysisResult.h>
#include <rarexsec/core/RegionAnalysis.h>
#include <rarexsec/data/RunConfig.h>
#include <rarexsec/core/SelectionRegistry.h>

namespace analysis {

class IAnalysisPlugin {
  public:
    virtual ~IAnalysisPlugin() = default;

    virtual void onInitialisation(AnalysisDefinition &def, const SelectionRegistry &sel_reg) = 0;

    virtual void onFinalisation(const AnalysisResult &results) = 0;
};

}
#endif
