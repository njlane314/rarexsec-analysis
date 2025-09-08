#pragma once
#include "PluginHost.h"
#include "IAnalysisPlugin.h"
#include "IPlotPlugin.h"
#include "AnalysisDataLoader.h"

namespace analysis {
using AnalysisPluginHost = PluginHost<IAnalysisPlugin, AnalysisDataLoader>;
using PlotPluginHost     = PluginHost<IPlotPlugin,     AnalysisDataLoader>;
}
