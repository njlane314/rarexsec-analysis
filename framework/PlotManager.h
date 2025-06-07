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

    inline void saveStackedPlot(const std::string& name, const AnalysisResult& result) {
        PlotStacked plot(name, result, output_dir_);
        plot.drawAndSave();
    }

private:
    std::string output_dir_;
};

}

#endif