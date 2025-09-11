#ifndef IANALYSIS_PLUGIN_H
#define IANALYSIS_PLUGIN_H

#include <nlohmann/json.hpp>

#include <rarexsec/app/AnalysisDefinition.h>
#include <rarexsec/app/AnalysisResult.h>
#include <rarexsec/app/RegionAnalysis.h>
#include <rarexsec/data/RunConfig.h>
#include <rarexsec/app/SelectionRegistry.h>

namespace analysis {

class IAnalysisPlugin {
  public:
    virtual ~IAnalysisPlugin() = default;

    virtual void onInitialisation(AnalysisDefinition &def, const SelectionRegistry &sel_reg) = 0;

    virtual void onFinalisation(const AnalysisResult &results) = 0;
};

}
#endif
