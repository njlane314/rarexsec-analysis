#pragma once
#include <rarexsec/plug/PluginHost.h>
#include <rarexsec/plug/IAnalysisPlugin.h>
#include <rarexsec/plug/IPlotPlugin.h>
#include <rarexsec/plug/ISystematicsPlugin.h>
#include <rarexsec/data/AnalysisDataLoader.h>
#include <rarexsec/syst/SystematicsProcessor.h>

namespace analysis {
using AnalysisPluginHost = PluginHost<IAnalysisPlugin, AnalysisDataLoader>;
using PlotPluginHost     = PluginHost<IPlotPlugin,     AnalysisDataLoader>;
using SystematicsPluginHost = PluginHost<ISystematicsPlugin, SystematicsProcessor>;
}
