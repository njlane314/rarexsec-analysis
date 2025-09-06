#ifndef IANALYSIS_PLUGIN_H
#define IANALYSIS_PLUGIN_H

#include <nlohmann/json.hpp>

#include "AnalysisDefinition.h"
#include "AnalysisResult.h"
#include "SelectionRegistry.h"

namespace analysis {

class IAnalysisPlugin {
  public:
    virtual ~IAnalysisPlugin() = default;

    virtual void onInitialisation(AnalysisDefinition &def, const SelectionRegistry &sel_reg) = 0;

    virtual void onFinalisation(const AnalysisResult &results) = 0;
};

}
#endif
