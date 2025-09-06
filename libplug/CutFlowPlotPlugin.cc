#include <cmath>
#include <nlohmann/json.hpp>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
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
            auto signal_keys = fetchSignalKeys(strat_reg, pc.signal_group);
            if (!signal_keys)
                continue;

            const auto &cf = res.region(RegionKey{pc.region}).cutFlow();

            auto stage_labels = std::vector<std::string>{pc.initial_label};
            stage_labels.insert(stage_labels.end(), pc.clauses.begin(), pc.clauses.end());

            auto counts = computeCounts(cf, pc.channel_column, *signal_keys);
            auto metrics = computeMetrics(cf, pc.channel_column, *signal_keys, counts.first, counts.second);

            SelectionEfficiencyPlot plot(pc.plot_name + "_" + pc.region, stage_labels, metrics.efficiencies,
                                         metrics.eff_errors, metrics.purities, metrics.pur_errors, pc.output_directory,
                                         pc.use_log_y);
            plot.drawAndSave("pdf");
            log::info("CutFlowPlotPlugin::run", pc.output_directory + "/" + pc.plot_name + "_" + pc.region + ".pdf");
        }
    }

  private:
    std::optional<std::vector<int>> fetchSignalKeys(StratifierRegistry &strat_reg, const std::string &group) const {
        try {
            return strat_reg.getSignalKeys(group);
        } catch (const std::exception &e) {
            log::error("CutFlowPlotPlugin::run", e.what());
            return std::nullopt;
        }
    }

    std::pair<double, double> computeCounts(const std::vector<RegionAnalysis::StageCount> &cf,
                                            const std::string &column, const std::vector<int> &signal_keys) const {
        double sig0 = 0.0;
        double sig0_w2 = 0.0;

        if (!cf.empty()) {
            auto scheme_it = cf[0].schemes.find(column);
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

        return {sig0, Neff0};
    }

    struct Metrics {
        std::vector<double> efficiencies;
        std::vector<double> eff_errors;
        std::vector<double> purities;
        std::vector<double> pur_errors;
    };

    Metrics computeMetrics(const std::vector<RegionAnalysis::StageCount> &cf, const std::string &column,
                           const std::vector<int> &signal_keys, double sig0, double Neff0) const {
        Metrics m;

        for (const auto &sc : cf) {
            double sig = 0.0;
            double sig_w2 = 0.0;

            auto scheme_it = sc.schemes.find(column);
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

            m.efficiencies.push_back(eff);
            m.eff_errors.push_back(eff_err);
            m.purities.push_back(pur);
            m.pur_errors.push_back(pur_err);
        }

        return m;
    }

    std::vector<PlotConfig> plots_;
};

}

#ifdef BUILD_PLUGIN
extern "C" analysis::IPlotPlugin *createPlotPlugin(const nlohmann::json &cfg) {
    return new analysis::CutFlowPlotPlugin(cfg);
}
#endif
