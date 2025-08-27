#ifndef ROC_CURVE_PLUGIN_H
#define ROC_CURVE_PLUGIN_H

#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <vector>

#include "AnalysisDataLoader.h"
#include "AnalysisLogger.h"
#include "HistogramCut.h"
#include "IAnalysisPlugin.h"
#include "RocCurvePlot.h"
#include "SelectionRegistry.h"
#include "StratifierRegistry.h"

#include "TH1D.h"

namespace analysis {

class RocCurvePlugin : public IAnalysisPlugin {
  public:
    struct PlotConfig {
        std::string region;
        std::string selection_rule;
        std::string channel_column;
        std::string signal_group;
        std::string variable;
        std::string output_directory{"plots"};
        std::string plot_name{"roc_curve"};
        int n_bins{100};
        double min{0.0};
        double max{1.0};
        CutDirection cut_direction{CutDirection::GreaterThan};
        std::vector<std::string> clauses;
    };

    explicit RocCurvePlugin(const nlohmann::json &cfg) {
        if (!cfg.contains("roc_curves") || !cfg.at("roc_curves").is_array()) {
            throw std::runtime_error("RocCurvePlugin missing roc_curves");
        }
        for (auto const &p : cfg.at("roc_curves")) {
            PlotConfig pc;
            pc.region = p.at("region").get<std::string>();
            pc.selection_rule = p.at("selection_rule").get<std::string>();
            pc.channel_column = p.at("channel_column").get<std::string>();
            pc.signal_group = p.at("signal_group").get<std::string>();
            pc.variable = p.at("variable").get<std::string>();
            pc.output_directory =
                p.value("output_directory", std::string{"plots"});
            pc.plot_name = p.value("plot_name", std::string{"roc_curve"});
            pc.n_bins = p.value("n_bins", 100);
            pc.min = p.value("min", 0.0);
            pc.max = p.value("max", 1.0);
            if (p.contains("cut_direction")) {
                auto dir = p.at("cut_direction").get<std::string>();
                pc.cut_direction = (dir == "LessThan")
                                       ? CutDirection::LessThan
                                       : CutDirection::GreaterThan;
            }
            plots_.push_back(std::move(pc));
        }
    }

    void onInitialisation(AnalysisDefinition &def,
                          const SelectionRegistry &sel_reg) override {
        for (auto &pc : plots_) {
            try {
                auto rule = sel_reg.getRule(pc.selection_rule);
                pc.clauses = rule.clauses;
                def.region(RegionKey{pc.region});
            } catch (const std::exception &e) {
                log::error("RocCurvePlugin::onInitialisation", e.what());
            }
        }
    }

    void onPreSampleProcessing(const SampleKey &, const RegionKey &,
                               const RunConfig &) override {}
    void onPostSampleProcessing(const SampleKey &, const RegionKey &,
                                const RegionAnalysisMap &) override {}

    void onFinalisation(const RegionAnalysisMap &) override {
        if (!loader_) {
            log::error("RocCurvePlugin::onFinalisation",
                       "No AnalysisDataLoader context provided");
            return;
        }

        StratifierRegistry strat_reg;
        for (const auto &pc : plots_) {
            std::vector<int> signal_keys;
            try {
                signal_keys = strat_reg.getSignalKeys(pc.signal_group);
            } catch (const std::exception &e) {
                log::error("RocCurvePlugin::onFinalisation", e.what());
                continue;
            }

            auto buildSignalExpr = [&](const std::vector<int> &keys) {
                std::string expr;
                for (size_t i = 0; i < keys.size(); ++i) {
                    if (i > 0)
                        expr += " || ";
                    expr +=
                        pc.channel_column + " == " + std::to_string(keys[i]);
                }
                return expr;
            };

            std::string signal_expr = buildSignalExpr(signal_keys);
            std::string selection_expr;
            for (size_t i = 0; i < pc.clauses.size(); ++i) {
                if (i > 0)
                    selection_expr += " && ";
                selection_expr += pc.clauses[i];
            }

            TH1D total_hist("total", "", pc.n_bins, pc.min, pc.max);
            TH1D sig_hist("sig", "", pc.n_bins, pc.min, pc.max);

            for (auto const &[skey, sample] : loader_->getSampleFrames()) {
                if (!sample.isMc())
                    continue;
                auto df = sample.nominal_node_;
                if (!selection_expr.empty())
                    df = df.Filter(selection_expr);
                auto tot_h =
                    df.Histo1D({"tot_h", "", pc.n_bins, pc.min, pc.max},
                               pc.variable, "nominal_event_weight");
                total_hist.Add(tot_h.GetPtr());
                auto sig_df = df.Filter(signal_expr);
                auto sig_h =
                    sig_df.Histo1D({"sig_h", "", pc.n_bins, pc.min, pc.max},
                                   pc.variable, "nominal_event_weight");
                sig_hist.Add(sig_h.GetPtr());
            }

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
                    double rej =
                        bkg_total > 0 ? 1.0 - (bkg_pass / bkg_total) : 0.0;
                    efficiencies.push_back(eff);
                    rejections.push_back(rej);
                }
            } else {
                for (int bin = 1; bin <= pc.n_bins; ++bin) {
                    double sig_pass = sig_hist.Integral(1, bin);
                    double bkg_pass = bkg_hist.Integral(1, bin);
                    double eff = sig_total > 0 ? sig_pass / sig_total : 0.0;
                    double rej =
                        bkg_total > 0 ? 1.0 - (bkg_pass / bkg_total) : 0.0;
                    efficiencies.push_back(eff);
                    rejections.push_back(rej);
                }
            }

            RocCurvePlot plot(pc.plot_name + "_" + pc.region, efficiencies,
                              rejections, pc.output_directory);
            plot.drawAndSave("pdf");
        }
    }

    static void setLoader(AnalysisDataLoader *loader) { loader_ = loader; }

  private:
    std::vector<PlotConfig> plots_;
    inline static AnalysisDataLoader *loader_ = nullptr;
};

}

#endif
