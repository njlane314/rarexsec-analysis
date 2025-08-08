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

    void onInitialisation(AnalysisDefinition&, const SelectionRegistry&) override {}
    void onPreSampleProcessing(const std::string&, const RegionConfig&, const std::string&) override {}
    void onPostSampleProcessing(const std::string&, const std::string&, const AnalysisResultMap&) override {}

    void onFinalisation(const AnalysisResultMap& results) override {
        gSystem->mkdir("plots", true);
        for (auto const& pc : plots_) {
            std::string result_key = pc.variable + "@" + pc.region;
            auto it = results.find(result_key);

            if (it == results.end()) {
                log::error("StackedHistogramPlugin", "Could not find analysis result for key:", result_key);
                continue;
            }
            
            StackedHistogramPlot plot(
                "stack_" + pc.variable + "_" + pc.region,
                it->second,
                pc.category_column,
                pc.output_directory,
                pc.overlay_signal,
                pc.cut_list,
                pc.annotate_numbers
            );
            plot.renderAndSave();
        }
    }

private:
    std::vector<PlotConfig> plots_;
};

}

#endif