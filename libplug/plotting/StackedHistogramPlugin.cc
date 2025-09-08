#include <stdexcept>
#include <string>
#include <vector>

#include "PluginRegistry.h"
#include <TSystem.h>

#include "AnalysisDataLoader.h"
#include "Logger.h"
#include "IPlotPlugin.h"
#include "StackedHistogramPlot.h"

namespace analysis {

class StackedHistogramPlugin : public IPlotPlugin {
  public:
    struct PlotConfig {
        std::string variable{};               // optional variable name
        std::string region{};                 // optional region key
        std::string category_column{"inclusive"};
        std::string output_directory = "plots";
        bool overlay_signal = true;
        std::vector<Cut> cut_list;
        bool annotate_numbers = true;
        bool use_log_y = false;
        std::string y_axis_label = "Events";
        bool selection_cuts = false;
        int n_bins{-1};
        double min{0.0};
        double max{0.0};
    };

    StackedHistogramPlugin(const PluginArgs &args, AnalysisDataLoader *) {
        const auto &cfg = args.plot_configs;
        if (cfg.contains("plots") && cfg.at("plots").is_array() && !cfg.at("plots").empty()) {
            for (auto const &p : cfg.at("plots")) {
                PlotConfig pc;
                pc.variable = p.value("variable", std::string());
                pc.region = p.value("region", std::string());
                pc.category_column = p.value("category_column", std::string("inclusive"));
                pc.output_directory = p.value("output_directory", std::string("plots"));
                pc.overlay_signal = p.value("overlay_signal", true);
                pc.annotate_numbers = p.value("annotate_numbers", true);
                pc.use_log_y = p.value("log_y", false);
                pc.y_axis_label = p.value("y_axis_label", "Events");
                pc.selection_cuts = p.value("selection_cuts", false);
                pc.n_bins = p.value("n_bins", -1);
                pc.min = p.value("min", 0.0);
                pc.max = p.value("max", 0.0);
                if (p.contains("cuts")) {
                    for (auto const &c : p.at("cuts")) {
                        auto dir = c.at("direction").get<std::string>() == "GreaterThan"
                                        ? CutDirection::GreaterThan
                                        : CutDirection::LessThan;
                        pc.cut_list.push_back({c.at("threshold").get<double>(), dir});
                    }
                }
                plots_.push_back(std::move(pc));
            }
        } else {
            // No explicit plot configuration provided; use defaults.
            plots_.push_back(PlotConfig{});
        }
    }

    void onPlot(const AnalysisResult &result) override {
        for (auto const &pc : plots_) {
            gSystem->mkdir(pc.output_directory.c_str(), true);
            std::vector<RegionKey> regions;
            if (pc.region.empty()) {
                for (const auto &kv : result.regions())
                    regions.emplace_back(kv.first);
            } else {
                regions.emplace_back(pc.region);
            }

            for (const auto &rkey : regions) {
                if (!result.regions().count(rkey)) {
                    log::error("StackedHistogramPlugin::onPlot", "Could not find region", rkey.str());
                    continue;
                }
                const auto &region_analysis = result.region(rkey);

                std::vector<VariableKey> variables;
                if (pc.variable.empty()) {
                    variables = region_analysis.getAvailableVariables();
                } else {
                    variables.emplace_back(pc.variable);
                }

                for (const auto &vkey : variables) {
                    // Fetch the variable result directly from the RegionAnalysis.
                    // In some scenarios the AnalysisResult's internal cache can
                    // miss entries even though the RegionAnalysis still holds
                    // them.  By querying the region object directly we ensure
                    // that valid variables are not mistakenly reported as
                    // missing.
                    if (!region_analysis.hasFinalVariable(vkey)) {
                        log::error("StackedHistogramPlugin::onPlot",
                                   "Could not find variable", vkey.str(),
                                   "in region", rkey.str());
                        continue;
                    }
                    const auto &variable_result =
                        region_analysis.getFinalVariable(vkey);
                    std::string plot_name =
                        "stack_" + IHistogramPlot::sanitise(vkey.str()) + "_" +
                        IHistogramPlot::sanitise(rkey.str());
                    StackedHistogramPlot plot(plot_name, variable_result,
                                              region_analysis, pc.category_column,
                                              pc.output_directory, pc.overlay_signal,
                                              pc.cut_list, pc.annotate_numbers,
                                              pc.use_log_y, pc.y_axis_label, pc.n_bins,
                                              pc.min, pc.max);
                    plot.drawAndSave("pdf");
                }
            }
        }
    }

  private:
    std::vector<PlotConfig> plots_;
};

} // namespace analysis

ANALYSIS_REGISTER_PLUGIN(analysis::IPlotPlugin, analysis::AnalysisDataLoader,
                         "StackedHistogramPlugin", analysis::StackedHistogramPlugin)

#ifdef BUILD_PLUGIN
extern "C" analysis::IPlotPlugin *createStackedHistogramPlugin(const analysis::PluginArgs &args) {
    return new analysis::StackedHistogramPlugin(args, nullptr);
}
extern "C" analysis::IPlotPlugin *createPlotPlugin(const analysis::PluginArgs &args) {
    return createStackedHistogramPlugin(args);
}
#endif
