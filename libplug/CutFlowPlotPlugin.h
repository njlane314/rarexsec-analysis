#ifndef CUTFLOWPLOTPLUGIN_H
#define CUTFLOWPLOTPLUGIN_H

#include <cmath>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <vector>

#include "AnalysisLogger.h"
#include "IPlotPlugin.h"
#include "SelectionEfficiencyPlot.h"
#include "StratifierRegistry.h"

namespace analysis {

class CutFlowPlotPlugin : public IPlotPlugin {
  public:
    struct PlotConfig {
        std::string selection_rule;
        std::string region;
        std::string signal_group;
        std::string channel_column;
        std::string initial_label;
        std::string plot_name;
        std::string output_directory{"plots"};
        bool use_log_y{false};
        std::vector<std::string> clauses;
    };

    explicit CutFlowPlotPlugin(const nlohmann::json &cfg) {
        if (!cfg.contains("plots") || !cfg.at("plots").is_array())
            throw std::runtime_error("CutFlowPlotPlugin missing plots");
        for (auto const &p : cfg.at("plots")) {
            PlotConfig pc;
            pc.selection_rule = p.at("selection_rule").get<std::string>();
            pc.region = p.at("region").get<std::string>();
            pc.signal_group = p.at("signal_group").get<std::string>();
            pc.channel_column = p.at("channel_column").get<std::string>();
            pc.initial_label = p.at("initial_label").get<std::string>();
            pc.plot_name = p.at("plot_name").get<std::string>();
            pc.output_directory = p.value("output_directory", std::string("plots"));
            pc.use_log_y = p.value("log_y", false);
            if (p.contains("clauses"))
                pc.clauses = p.at("clauses").get<std::vector<std::string>>();
            plots_.push_back(std::move(pc));
        }
    }

    void run(const AnalysisResult &res) override {
        StratifierRegistry strat_reg;
        for (const auto &pc : plots_) {
            std::vector<int> signal_keys;
            try {
                signal_keys = strat_reg.getSignalKeys(pc.signal_group);
            } catch (const std::exception &e) {
                log::error("CutFlowPlotPlugin::run", e.what());
                continue;
            }

            std::vector<std::string> stage_labels{pc.initial_label};
            stage_labels.insert(stage_labels.end(), pc.clauses.begin(), pc.clauses.end());

            const auto &cf = res.region(RegionKey{pc.region}).cutFlow();

            double sig0 = 0.0;
            double sig0_w2 = 0.0;
            if (!cf.empty()) {
                auto scheme_it = cf[0].schemes.find(pc.channel_column);
                if (scheme_it != cf[0].schemes.end())
                    for (int key : signal_keys) {
                        auto it = scheme_it->second.find(key);
                        if (it != scheme_it->second.end()) {
                            sig0 += it->second.first;
                            sig0_w2 += it->second.second;
                        }
                    }
            }
            double Neff0 = sig0_w2 > 0 ? (sig0 * sig0) / sig0_w2 : 0.0;

            std::vector<double> efficiencies, eff_errors, purities, pur_errors;
            for (const auto &sc : cf) {
                double sig = 0.0;
                double sig_w2 = 0.0;
                auto scheme_it = sc.schemes.find(pc.channel_column);
                if (scheme_it != sc.schemes.end())
                    for (int key : signal_keys) {
                        auto it = scheme_it->second.find(key);
                        if (it != scheme_it->second.end()) {
                            sig += it->second.first;
                            sig_w2 += it->second.second;
                        }
                    }
                double eff = sig0 > 0 ? sig / sig0 : 0.0;
                double eff_err = Neff0 > 0 ? std::sqrt(eff * (1.0 - eff) / Neff0) : 0.0;
                double tot = sc.total;
                double tot_w2 = sc.total_w2;
                double pur = tot > 0 ? sig / tot : 0.0;
                double Neff_tot = tot_w2 > 0 ? (tot * tot) / tot_w2 : 0.0;
                double pur_err = Neff_tot > 0 ? std::sqrt(pur * (1.0 - pur) / Neff_tot) : 0.0;
                efficiencies.push_back(eff);
                eff_errors.push_back(eff_err);
                purities.push_back(pur);
                pur_errors.push_back(pur_err);
            }

            SelectionEfficiencyPlot plot(pc.plot_name + "_" + pc.region, stage_labels, efficiencies, eff_errors,
                                         purities, pur_errors, pc.output_directory, pc.use_log_y);
            plot.drawAndSave("pdf");
            log::info("CutFlowPlotPlugin::run", pc.output_directory + "/" + pc.plot_name + "_" + pc.region + ".pdf");
        }
    }

  private:
    std::vector<PlotConfig> plots_;
};

} // namespace analysis

#ifdef BUILD_PLUGIN
extern "C" analysis::IPlotPlugin *createPlotPlugin(const nlohmann::json &cfg) {
    return new analysis::CutFlowPlotPlugin(cfg);
}
#endif

#endif
