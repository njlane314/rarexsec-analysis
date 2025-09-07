#ifndef IPLOT_PLUGIN_H
#define IPLOT_PLUGIN_H

#include "AnalysisResult.h"

namespace analysis {

class IPlotPlugin {
  public:
    virtual ~IPlotPlugin() = default;

    virtual void onPlot(const AnalysisResult &result) = 0;
};

}

#endif
