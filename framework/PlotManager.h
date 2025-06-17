#ifndef PLOTMANAGER_H
#define PLOTMANAGER_H

#include <string>
#include <map>
#include <stdexcept>
#include <algorithm>
#include "AnalysisRunner.h"
#include "AnalysisResult.h"
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

    inline void saveStackedPlot(
        const AnalysisPhaseSpace& results,
        const std::string& variable_name,
        const std::string& region_name,
        const std::string& analysis_channel_column) {
        
        const AnalysisResult& result = this->getAnalysisResult(results, variable_name, region_name);

        std::string sanitized_var_name = this->cleanFilename(variable_name);
        std::string sanitized_region_key = this->cleanFilename(region_name.empty() ? "default" : region_name);
        std::string plot_filename = "stacked_" + sanitized_var_name + "_" + sanitized_region_key;
        
        PlotStacked plot(plot_filename, result, analysis_channel_column, output_dir_, true);
        plot.drawAndSave();
    }

private:
    std::string output_dir_;

    inline const AnalysisResult& getAnalysisResult(
        const AnalysisPhaseSpace& results,
        const std::string& variable_name,
        const std::string& region_name) const {
        
        std::string plot_key = variable_name + "@" + region_name;
        
        if (results.count(plot_key)) {
            return results.at(plot_key);
        } else {
            std::string error_msg = "Error: Analysis result for variable '";
            error_msg += variable_name;
            error_msg += "' and region '";
            error_msg += region_name;
            error_msg += "' (key: '";
            error_msg += plot_key;
            error_msg += "') not found in the AnalysisPhaseSpace map.";
            throw std::runtime_error(error_msg);
        }
    }

    inline std::string cleanFilename(std::string s) const {
        std::replace(s.begin(), s.end(), '.', '_');
        std::replace(s.begin(), s.end(), '/', '_');
        std::replace(s.begin(), s.end(), ' ', '_');
        return s;
    }
};

}

#endif