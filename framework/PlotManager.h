#ifndef PLOTMANAGER_H
#define PLOTMANAGER_H

#include <string>
#include <map>
#include "AnalysisRunner.h"
#include "AnalysisSpace.h"
#include "Histogram.h"

#include "PlotStacked.h"
#include "TSystem.h"

namespace AnalysisFramework {

class PlotManager {
public:
    inline PlotManager(const std::string& output_dir = "plots") 
        : output_dir_(output_dir) {
        gSystem->mkdir(output_dir_.c_str(), true);
    }

    inline void saveStackedPlot(const std::string& name, const AnalysisResult& result, const std::string& analysis_channel_column) {
        PlotStacked plot(name, result, analysis_channel_column, output_dir_, true);
        plot.drawAndSave();
    }

private:
    std::string output_dir_;
};

}

#endif