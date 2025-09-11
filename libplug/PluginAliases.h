#pragma once
#include "PluginHost.h"
#include "IAnalysisPlugin.h"
#include "IPlotPlugin.h"
#include "ISystematicsPlugin.h"
#include "AnalysisDataLoader.h"
#include "SystematicsProcessor.h"

namespace analysis {
using AnalysisPluginHost = PluginHost<IAnalysisPlugin, AnalysisDataLoader>;
using PlotPluginHost     = PluginHost<IPlotPlugin,     AnalysisDataLoader>;
using SystematicsPluginHost = PluginHost<ISystematicsPlugin, SystematicsProcessor>;
}
