#ifndef STACKEDHISTOGRAMPLUGIN_H
#define STACKEDHISTOGRAMPLUGIN_H

#include <map>
#include <regex>
#include <stdexcept>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include <TSystem.h>

#include "AnalysisLogger.h"
#include "IAnalysisPlugin.h"
#include "StackedHistogramPlot.h"

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
        bool selection_cuts = false;
        int n_bins{-1};
        double min{0.0};
        double max{0.0};
    };

    explicit StackedHistogramPlugin(const nlohmann::json &cfg) {
        if (!cfg.contains("plots") || !cfg.at("plots").is_array())
            throw std::runtime_error("StackedHistogramPlugin missing plots");
        for (auto const &p : cfg.at("plots")) {
            PlotConfig pc;
            pc.variable = p.at("variable").get<std::string>();
            pc.region = p.at("region").get<std::string>();
            pc.category_column = p.value("category_column", std::string());
            pc.output_directory =
                p.value("output_directory", std::string("plots"));
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
                    auto dir =
                        c.at("direction").get<std::string>() == "GreaterThan"
                            ? CutDirection::GreaterThan
                            : CutDirection::LessThan;
                    pc.cut_list.push_back(
                        {c.at("threshold").get<double>(), dir});
                }
            }
            plots_.push_back(std::move(pc));
        }
    }

    void onInitialisation(AnalysisDefinition &def,
                          const SelectionRegistry &) override {
        for (auto const &pc : plots_) {
            if (!pc.selection_cuts)
                continue;
            RegionKey rkey{pc.region};
            if (region_cuts_.count(rkey))
                continue;
            try {
                const auto &sel_str = def.region(rkey).selection().str();
                parseSelectionCuts(rkey, sel_str);
            } catch (const std::exception &e) {
                log::error("StackedHistogramPlugin::onInitialisation",
                           "Could not parse selection for region", rkey.str(),
                           e.what());
            }
        }
    }
    void onPreSampleProcessing(const SampleKey &, const RegionKey &,
                               const RunConfig &) override {}
    void onPostSampleProcessing(const SampleKey &, const RegionKey &,
                                const RegionAnalysisMap &) override {}

    void onFinalisation(const RegionAnalysisMap &region_map) override {
        gSystem->mkdir("plots", true);
        for (auto const &pc : plots_) {
            RegionKey rkey{pc.region};
            auto it = region_map.find(rkey);

            if (it == region_map.end()) {
                log::error(
                    "StackedHistogramPlugin::onFinalisation",
                    "Could not find analysis region for key:", rkey.str());
                continue;
            }

            VariableKey vkey{pc.variable};
            if (!it->second.hasFinalVariable(vkey)) {
                log::error("StackedHistogramPlugin::onFinalisation",
                           "Could not find variable", vkey.str(), "in region",
                           rkey.str());
                continue;
            }

            const auto &region_analysis = it->second;
            const auto &variable_result =
                region_analysis.getFinalVariable(vkey);

            std::vector<Cut> cuts = pc.cut_list;
            if (pc.selection_cuts) {
                auto r_it = region_cuts_.find(rkey);
                if (r_it != region_cuts_.end()) {
                    auto c_it = r_it->second.find(pc.variable);
                    if (c_it != r_it->second.end()) {
                        cuts.insert(cuts.end(), c_it->second.begin(),
                                    c_it->second.end());
                    }
                }
            }

            StackedHistogramPlot plot(
                "stack_" + pc.variable + "_" + pc.region, variable_result,
                region_analysis, pc.category_column, pc.output_directory,
                pc.overlay_signal, cuts, pc.annotate_numbers, pc.use_log_y,
                pc.y_axis_label, pc.n_bins, pc.min, pc.max);
            plot.drawAndSave("pdf");
        }
    }

  private:
    void parseSelectionCuts(const RegionKey &region, const std::string &expr) {
        static const std::regex rgx(
            R"((\w+)\s*([<>])=?\s*(-?\d*\.?\d+(?:[eE][-+]?\d+)?))");
        auto begin = std::sregex_iterator(expr.begin(), expr.end(), rgx);
        auto end = std::sregex_iterator();
        for (auto it = begin; it != end; ++it) {
            std::string var = (*it)[1];
            std::string op = (*it)[2];
            double thr = std::stod((*it)[3]);
            CutDirection dir =
                op == ">" ? CutDirection::GreaterThan : CutDirection::LessThan;
            region_cuts_[region][var].push_back({thr, dir});
        }
    }

    std::vector<PlotConfig> plots_;
    std::map<RegionKey, std::map<std::string, std::vector<Cut>>> region_cuts_;
};

}

#endif
