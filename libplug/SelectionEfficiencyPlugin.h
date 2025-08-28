#ifndef SELECTION_EFFICIENCY_PLUGIN_H
#define SELECTION_EFFICIENCY_PLUGIN_H

#include <cmath>
#include <map>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "AnalysisLogger.h"
#include "IAnalysisPlugin.h"
#include "SelectionEfficiencyPlot.h"
#include "SelectionRegistry.h"
#include "StratifierRegistry.h"

namespace analysis {

class SelectionEfficiencyPlugin : public IAnalysisPlugin {
  public:
    SelectionEfficiencyPlugin(const nlohmann::json & acfg, const nlohmann::json &pcfg)
        : config_analysis(acfg), config_plot(pcfg) {}

    void onInitialisation(AnalysisDefinition &def, const SelectionRegistry &sel_reg) override {
        for (auto &pc : plots_) {
            try {
                if (pc.clauses.empty()) {
                    auto rule = sel_reg.getRule(pc.selection_rule);
                    pc.clauses = rule.clauses;
                }

                def.region(RegionKey{pc.region});
            } catch (const std::exception &e) {
                log::error("SelectionEfficiencyPlugin::onInitialisation", e.what());
            }
        }
    }

    void onPreSampleProcessing(const SampleKey &, const RegionKey &, const RunConfig &) override {}
    void onPostSampleProcessing(const SampleKey &, const RegionKey &, const RegionAnalysisMap &) override {}

    void onFinalisation(const AnalysisResult &res) override {
        StratifierRegistry strat_reg;

        for (const auto &pc : plots_) {
            std::vector<int> signal_keys;
            try {
                signal_keys = strat_reg.getSignalKeys(pc.signal_group);
            } catch (const std::exception &e) {
                log::error("SelectionEfficiencyPlugin::onFinalisation", e.what());
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
            log::info("SelectionEfficiencyPlugin::onFinalisation",
                      pc.output_directory + "/" + pc.plot_name + "_" + pc.region + ".pdf");
        }
    }

  private:
    std::vector<PlotConfig> plots_;
};

} // namespace analysis

#ifdef BUILD_PLUGIN
extern "C" analysis::IAnalysisPlugin *createPlugin(const nlohmann::json &cfg) {
    return new analysis::SelectionEfficiencyPlugin(cfg);
}
#endif

#endif
