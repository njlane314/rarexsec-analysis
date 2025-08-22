#ifndef STACKEDHISTOGRAMPLUGIN_H
#define STACKEDHISTOGRAMPLUGIN_H

#include "IAnalysisPlugin.h"
#include "StackedHistogramPlot.h"
#include <nlohmann/json.hpp>
#include <vector>
#include <string>
#include <stdexcept>
#include <TSystem.h>
#include "Logger.h"

namespace analysis {

class StackedHistogramPlugin : public IAnalysisPlugin {
public:
    struct PlotConfig {
        std::string variable;
        std::string region;
        std::string category_column;
        std::string output_directory = "plots";
        bool overlay_signal = true;
        std::vector<Cut> cut_list;
        bool annotate_numbers = true;
        bool use_log_y = false;
        std::string y_axis_label = "Events";
    };

    explicit StackedHistogramPlugin(const nlohmann::json& cfg) {
        if (!cfg.contains("plots") || !cfg.at("plots").is_array())
            throw std::runtime_error("StackedHistogramPlugin missing plots");
        for (auto const& p : cfg.at("plots")) {
            PlotConfig pc;
            pc.variable         = p.at("variable").get<std::string>();
            pc.region           = p.at("region").get<std::string>();
            pc.category_column  = p.value("category_column", std::string());
            pc.output_directory = p.value("output_directory", std::string("plots"));
            pc.overlay_signal   = p.value("overlay_signal", true);
            pc.annotate_numbers = p.value("annotate_numbers", true);
            pc.use_log_y        = p.value("log_y", false);
            pc.y_axis_label     = p.value("y_axis_label", "Events");
            if (p.contains("cuts")) {
                for (auto const& c : p.at("cuts")) {
                    auto dir = c.at("direction").get<std::string>() == "GreaterThan"
                                   ? CutDirection::GreaterThan
                                   : CutDirection::LessThan;
                    pc.cut_list.push_back({c.at("threshold").get<double>(), dir});
                }
            }
            plots_.push_back(std::move(pc));
        }
    }

    void onFinalisation(const AnalysisRegionMap& region_map) override {
        gSystem->mkdir("plots", true);
        for (auto const& pc : plots_) {
            RegionKey rkey{pc.region};
            auto it = region_map.find(rkey);

            if (it == region_map.end()) {
                log::error("StackedHistogramPlugin", "Could not find analysis region for key:", rkey.str());
                continue;
            }
            
            VariableKey vkey{pc.variable};
            if (!it->second.hasFinalVariable(vkey)) {
                log::error("StackedHistogramPlugin", "Could not find variable", vkey.str(), "in region", rkey.str());
                continue;
            }
            
            const auto& region_analysis = it->second;
            const auto& variable_result = region_analysis.getFinalVariable(vkey);

            StackedHistogramPlot plot(
                "stack_" + pc.variable + "_" + pc.region,
                variable_result,
                region_analysis,
                pc.category_column,
                pc.output_directory,
                pc.overlay_signal,
                pc.cut_list,
                pc.annotate_numbers,
                pc.use_log_y,
                pc.y_axis_label
            );
            plot.drawAndSave();
        }
    }

private:
    std::vector<PlotConfig> plots_;
};

}

#endif