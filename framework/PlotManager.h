#ifndef PLOTMANAGER_H
#define PLOTMANAGER_H

#include <string>
#include "AnalysisRunner.h"
#include "Histogram.h"

#include "PlotStacked.h"

namespace AnalysisFramework {

class PlotManager {
public:
    inline PlotManager(const std::string& output_dir = "plots") 
        : output_dir_(output_dir) {}

    inline void saveStackedPlot(const std::string& name, const AnalysisResult& result) {
        PlotStacked plot(name, result, output_dir_);
        plot.drawAndSave();
    }

private:
    std::string output_dir_;
};

}

#endif