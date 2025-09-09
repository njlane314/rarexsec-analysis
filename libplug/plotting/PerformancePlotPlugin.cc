#include <stdexcept>
#include <string>
#include <vector>
#include <utility>
#include <cmath>

#include "PluginRegistry.h"
#include "AnalysisDataLoader.h"
#include "Logger.h"
#include "HistogramCut.h"
#include "IPlotPlugin.h"
#include "PerformancePlot.h"
#include "SignificanceImprovementPlot.h"
#include "StratifierRegistry.h"

#include "TH1D.h"

namespace analysis {

class PerformancePlotPlugin : public IPlotPlugin {
  public:
    struct PlotConfig {
        std::string region;
        std::string selection_rule;
        std::string channel_column;
        std::string signal_group;
        std::string variable;
        std::string output_directory{"plots"};
        std::string plot_name{"performance_plot"};
        int n_bins{100};
        double min{0.0};
        double max{1.0};
        CutDirection cut_direction{CutDirection::GreaterThan};
        std::vector<std::string> clauses;
    };

    PerformancePlotPlugin(const PluginArgs &args, AnalysisDataLoader *loader) : loader_(loader) {
        const auto &cfg = args.plot_configs;
        if (!cfg.contains("performance_plots") || !cfg.at("performance_plots").is_array()) {
            throw std::runtime_error("PerformancePlotPlugin missing performance_plots");
        }
        for (auto const &p : cfg.at("performance_plots")) {
            PlotConfig pc;
            pc.region = p.at("region").get<std::string>();
            pc.selection_rule = p.value("selection_rule", std::string());
            pc.channel_column = p.at("channel_column").get<std::string>();
            pc.signal_group = p.at("signal_group").get<std::string>();
            pc.variable = p.at("variable").get<std::string>();
            pc.output_directory = p.value("output_directory", std::string{"plots"});
            pc.plot_name = p.value("plot_name", std::string{"performance_plot"});
            pc.n_bins = p.value("n_bins", 100);
            pc.min = p.value("min", 0.0);
            pc.max = p.value("max", 1.0);
            if (p.contains("cut_direction")) {
                auto dir = p.at("cut_direction").get<std::string>();
                pc.cut_direction = (dir == "LessThan") ? CutDirection::LessThan : CutDirection::GreaterThan;
            }
            plots_.push_back(std::move(pc));
        }
    }

    void onPlot(const AnalysisResult &) override {
        if (!loader_) {
            log::error("PerformancePlotPlugin::onPlot", "No AnalysisDataLoader context provided");
            return;
        }

        StratifierRegistry strat_reg;
        for (const auto &pc : plots_) {
            std::string signal_expr;
            std::string selection_expr;

            if (!this->buildExpressions(pc, strat_reg, signal_expr, selection_expr))
                continue;

            auto [total_hist, sig_hist] = this->accumulateHistograms(pc, signal_expr, selection_expr);

            auto [efficiencies, rejections] = this->computePerformancePoints(pc, total_hist, sig_hist);
            auto sic = this->computeSIC(efficiencies, rejections);
            double auc = this->computeAUC(efficiencies, rejections);

            this->renderPlot(pc, efficiencies, rejections, auc, sic);
        }
    }

    static void setLegacyLoader(AnalysisDataLoader *ldr) { legacy_loader_ = ldr; }
    static AnalysisDataLoader *legacyLoader() { return legacy_loader_; }

  private:
    bool buildExpressions(const PlotConfig &pc, StratifierRegistry &strat_reg, std::string &signal_expr,
                          std::string &selection_expr) const {
        std::vector<int> signal_keys;

        try {
            signal_keys = strat_reg.getSignalKeys(pc.signal_group);
        } catch (const std::exception &e) {
            log::error("PerformancePlotPlugin::onPlot", e.what());
            return false;
        }

        for (size_t i = 0; i < signal_keys.size(); ++i) {
            if (i > 0)
                signal_expr += " || ";
            signal_expr += pc.channel_column + " == " + std::to_string(signal_keys[i]);
        }

        for (size_t i = 0; i < pc.clauses.size(); ++i) {
            if (i > 0)
                selection_expr += " && ";
            selection_expr += pc.clauses[i];
        }

        return true;
    }

    std::pair<TH1D, TH1D> accumulateHistograms(const PlotConfig &pc, const std::string &signal_expr,
                                               const std::string &selection_expr) const {
        TH1D total_hist("total", "", pc.n_bins, pc.min, pc.max);
        TH1D sig_hist("sig", "", pc.n_bins, pc.min, pc.max);

        for (auto const &[skey, sample] : loader_->getSampleFrames()) {
            if (!sample.isMc())
                continue;

            auto df = sample.nominal_node_;
            if (!selection_expr.empty())
                df = df.Filter(selection_expr);

            auto tot_h = df.Histo1D({"tot_h", "", pc.n_bins, pc.min, pc.max}, pc.variable, "nominal_event_weight");
            total_hist.Add(tot_h.GetPtr());

            auto sig_df = df.Filter(signal_expr);
            auto sig_h = sig_df.Histo1D({"sig_h", "", pc.n_bins, pc.min, pc.max}, pc.variable, "nominal_event_weight");
            sig_hist.Add(sig_h.GetPtr());
        }

        return {total_hist, sig_hist};
    }

    std::pair<std::vector<double>, std::vector<double>> computePerformancePoints(const PlotConfig &pc, const TH1D &total_hist,
                                                                         const TH1D &sig_hist) const {
        TH1D bkg_hist(total_hist);
        bkg_hist.Add(&sig_hist, -1.0);

        double sig_total = sig_hist.Integral();
        double bkg_total = bkg_hist.Integral();

        std::vector<double> efficiencies;
        std::vector<double> rejections;

        if (pc.cut_direction == CutDirection::GreaterThan) {
            for (int bin = pc.n_bins; bin >= 1; --bin) {
                double sig_pass = sig_hist.Integral(bin, pc.n_bins);
                double bkg_pass = bkg_hist.Integral(bin, pc.n_bins);

                double eff = sig_total > 0 ? sig_pass / sig_total : 0.0;
                double rej = bkg_total > 0 ? 1.0 - (bkg_pass / bkg_total) : 0.0;

                efficiencies.push_back(eff);
                rejections.push_back(rej);
            }
        } else {
            for (int bin = 1; bin <= pc.n_bins; ++bin) {
                double sig_pass = sig_hist.Integral(1, bin);
                double bkg_pass = bkg_hist.Integral(1, bin);

                double eff = sig_total > 0 ? sig_pass / sig_total : 0.0;
                double rej = bkg_total > 0 ? 1.0 - (bkg_pass / bkg_total) : 0.0;

                efficiencies.push_back(eff);
                rejections.push_back(rej);
            }
        }

        return {efficiencies, rejections};
    }

    std::vector<double> computeSIC(const std::vector<double> &efficiencies,
                                   const std::vector<double> &rejections) const {
        std::vector<double> sic;
        sic.reserve(efficiencies.size());
        for (size_t i = 0; i < efficiencies.size(); ++i) {
            double bkg_eff = 1.0 - rejections[i];
            double val = bkg_eff > 0.0 ? efficiencies[i] / std::sqrt(bkg_eff) : 0.0;
            sic.push_back(val);
        }
        return sic;
    }

    double computeAUC(const std::vector<double> &efficiencies,
                      const std::vector<double> &rejections) const {
        if (efficiencies.size() < 2)
            return 0.0;
        double auc = 0.0;
        for (size_t i = 1; i < efficiencies.size(); ++i) {
            double x1 = 1.0 - rejections[i - 1];
            double x2 = 1.0 - rejections[i];
            double y1 = efficiencies[i - 1];
            double y2 = efficiencies[i];
            auc += 0.5 * (x2 - x1) * (y1 + y2);
        }
        return auc;
    }

    void renderPlot(const PlotConfig &pc, const std::vector<double> &efficiencies,
                    const std::vector<double> &rejections, double auc,
                    const std::vector<double> &sic) const {
        PerformancePlot roc_plot(pc.plot_name + "_" + pc.region, efficiencies, rejections, pc.output_directory, auc);
        roc_plot.drawAndSave("pdf");
        SignificanceImprovementPlot sic_plot(pc.plot_name + "_sic_" + pc.region, efficiencies, sic, pc.output_directory);
        sic_plot.drawAndSave("pdf");
    }

    std::vector<PlotConfig> plots_;
    AnalysisDataLoader *loader_;
    inline static AnalysisDataLoader *legacy_loader_ = nullptr;
};

} // namespace analysis

ANALYSIS_REGISTER_PLUGIN(analysis::IPlotPlugin, analysis::AnalysisDataLoader,
                         "PerformancePlotPlugin", analysis::PerformancePlotPlugin)

#ifdef BUILD_PLUGIN
extern "C" analysis::IPlotPlugin *createPerformancePlotPlugin(const analysis::PluginArgs &args) {
    return new analysis::PerformancePlotPlugin(args, analysis::PerformancePlotPlugin::legacyLoader());
}
extern "C" analysis::IPlotPlugin *createPlotPlugin(const analysis::PluginArgs &args) {
    return createPerformancePlotPlugin(args);
}
extern "C" void setPluginContext(analysis::AnalysisDataLoader *loader) {
    analysis::PerformancePlotPlugin::setLegacyLoader(loader);
}
#endif
