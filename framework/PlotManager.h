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

    inline void saveAllStackedPlots(
        const std::map<std::string, AnalysisResult>& results,
        const AnalysisSpace& analysis_space) {

        const auto& regions = analysis_space.getRegions();
        const auto& variables = analysis_space.getVariables();

        for (const auto& [reg_name, reg_props] : regions) {
            std::string region_output_dir = output_dir_ + "/" + reg_name;
            PlotManager region_plot_manager(region_output_dir);

            for (const auto& [var_name, var_props] : variables) {
                std::string task_id = var_name + "@" + reg_name;
                if (results.count(task_id)) {
                    region_plot_manager.saveStackedPlot(var_name, results.at(task_id));
                }
            }
        }
    }

private:
    std::string output_dir_;
};

}

#endif