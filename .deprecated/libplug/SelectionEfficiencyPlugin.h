#ifndef SELECTION_EFFICIENCY_PLUGIN_H
#define SELECTION_EFFICIENCY_PLUGIN_H

#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <vector>

#include "AnalysisDataLoader.h"
#include "AnalysisLogger.h"
#include "IAnalysisPlugin.h"
#include "SelectionEfficiencyPlot.h"
#include "SelectionRegistry.h"
#include "StratifierRegistry.h"

namespace analysis {

class SelectionEfficiencyPlugin : public IAnalysisPlugin {
  public:
    struct PlotConfig {
        std::string region;
        std::string selection_rule;
        std::string channel_column;
        std::string signal_group;
        std::string output_directory{"plots"};
        std::string plot_name{"selection_efficiency"};
        bool use_log_y{false};
        std::vector<std::string> clauses;
    };

    explicit SelectionEfficiencyPlugin(const nlohmann::json &cfg) {
        if (!cfg.contains("efficiency_plots") || !cfg.at("efficiency_plots").is_array()) {
            throw std::runtime_error("SelectionEfficiencyPlugin missing efficiency_plots");
        }
        for (auto const &p : cfg.at("efficiency_plots")) {
            PlotConfig pc;
            pc.region = p.at("region").get<std::string>();
            pc.selection_rule = p.at("selection_rule").get<std::string>();
            pc.channel_column = p.at("channel_column").get<std::string>();
            pc.signal_group = p.at("signal_group").get<std::string>();
            pc.output_directory = p.value("output_directory", std::string{"plots"});
            pc.plot_name = p.value("plot_name", std::string{"selection_efficiency"});
            pc.use_log_y = p.value("log_y", false);
            plots_.push_back(std::move(pc));
        }
    }

    void onInitialisation(AnalysisDefinition &def, const SelectionRegistry &sel_reg) override {
        for (auto &pc : plots_) {
            try {
                auto rule = sel_reg.getRule(pc.selection_rule);
                pc.clauses = rule.clauses;

                def.region(RegionKey{pc.region});
            } catch (const std::exception &e) {
                log::error("SelectionEfficiencyPlugin::onInitialisation", e.what());
            }
        }
    }

    void onPreSampleProcessing(const SampleKey &, const RegionKey &, const RunConfig &) override {}
    void onPostSampleProcessing(const SampleKey &, const RegionKey &, const RegionAnalysisMap &) override {}

    void onFinalisation(const RegionAnalysisMap &) override {
        if (!loader_) {
            log::error("SelectionEfficiencyPlugin::onFinalisation", "No AnalysisDataLoader context provided");
            return;
        }

        StratifierRegistry strat_reg;

        for (const auto &pc : plots_) {
            std::vector<int> signal_keys;
            try {
                signal_keys = strat_reg.getSignalKeys(pc.signal_group);
            } catch (const std::exception &e) {
                log::error("SelectionEfficiencyPlugin::onFinalisation", e.what());
                continue;
            }

            auto buildSignalExpr = [&](const std::vector<int> &keys) {
                std::string expr;
                for (size_t i = 0; i < keys.size(); ++i) {
                    if (i > 0)
                        expr += " || ";
                    expr += pc.channel_column + " == " + std::to_string(keys[i]);
                }
                return expr;
            };

            std::string signal_expr = buildSignalExpr(signal_keys);

            std::vector<std::string> stage_labels{"All Events"};
            stage_labels.insert(stage_labels.end(), pc.clauses.begin(), pc.clauses.end());

            std::vector<std::string> cumulative_filters{""};
            std::string current;
            for (const auto &clause : pc.clauses) {
                if (!current.empty())
                    current += " && ";
                current += clause;
                cumulative_filters.push_back(current);
            }

            struct CountInfo {
                double sig = 0.0;
                double sig_w2 = 0.0;
                double tot = 0.0;
                double tot_w2 = 0.0;
            };
            std::vector<CountInfo> counts(cumulative_filters.size());

            for (const auto &[skey, sample] : loader_->getSampleFrames()) {
                if (!sample.isMc())
                    continue;
                for (size_t i = 0; i < cumulative_filters.size(); ++i) {
                    auto df = sample.nominal_node_;
                    if (!cumulative_filters[i].empty())
                        df = df.Filter(cumulative_filters[i]);
                    auto df_w2 = df.Define("w2", "nominal_event_weight*nominal_event_weight");
                    auto tot_w = df_w2.Sum<double>("nominal_event_weight");
                    auto tot_w2 = df_w2.Sum<double>("w2");
                    auto sig_df = df_w2.Filter(signal_expr);
                    auto sig_w = sig_df.Sum<double>("nominal_event_weight");
                    auto sig_w2 = sig_df.Sum<double>("w2");
                    counts[i].tot += tot_w.GetValue();
                    counts[i].tot_w2 += tot_w2.GetValue();
                    counts[i].sig += sig_w.GetValue();
                    counts[i].sig_w2 += sig_w2.GetValue();
                }
            }

            double sig0 = counts[0].sig;
            double sig0_w2 = counts[0].sig_w2;
            double Neff0 = sig0_w2 > 0 ? (sig0 * sig0) / sig0_w2 : 0.0;

            std::vector<double> efficiencies, eff_errors, purities, pur_errors;
            for (const auto &c : counts) {
                double eff = sig0 > 0 ? c.sig / sig0 : 0.0;
                double eff_err = (Neff0 > 0) ? std::sqrt(eff * (1.0 - eff) / Neff0) : 0.0;
                double pur = c.tot > 0 ? c.sig / c.tot : 0.0;
                double Neff_tot = c.tot_w2 > 0 ? (c.tot * c.tot) / c.tot_w2 : 0.0;
                double pur_err = (Neff_tot > 0) ? std::sqrt(pur * (1.0 - pur) / Neff_tot) : 0.0;
                efficiencies.push_back(eff);
                eff_errors.push_back(eff_err);
                purities.push_back(pur);
                pur_errors.push_back(pur_err);
            }

            SelectionEfficiencyPlot plot(pc.plot_name + "_" + pc.region, stage_labels, efficiencies, eff_errors,
                                         purities, pur_errors, pc.output_directory, pc.use_log_y);
            plot.drawAndSave("pdf");
        }
    }

    static void setLoader(AnalysisDataLoader *loader) { loader_ = loader; }

  private:
    std::vector<PlotConfig> plots_;
    inline static AnalysisDataLoader *loader_ = nullptr;
};

} // namespace analysis

#ifdef BUILD_PLUGIN
extern "C" analysis::IAnalysisPlugin *createPlugin(const nlohmann::json &cfg) {
    return new analysis::SelectionEfficiencyPlugin(cfg);
}
extern "C" void setPluginContext(analysis::AnalysisDataLoader *loader) {
    analysis::SelectionEfficiencyPlugin::setLoader(loader);
}
#endif

#endif
