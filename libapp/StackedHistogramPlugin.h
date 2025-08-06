// StackedHistogramPlugin.h
#ifndef STACKEDHISTOGRAMPLUGIN_H
#define STACKEDHISTOGRAMPLUGIN_H

#include "IAnalysisPlugin.h"
#include "StackedHistogramPlot.h"
#include <nlohmann/json.hpp>
#include <vector>
#include <string>
#include <stdexcept>
#include <TSystem.h>

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
    void onPostSampleProcessing(const std::string&, const std::string&, const HistogramResult&) override {}

    void onFinalisation(const HistogramResult& results) override {
        gSystem->mkdir("plots", true);
        for (auto const& pc : plots_) {
            std::string name = "stack_" + pc.variable + "_" + pc.region;
            StackedHistogramPlot plot(
                name,
                results,
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

} // namespace analysis

#endif // STACKEDHISTOGRAMPLUGIN_H