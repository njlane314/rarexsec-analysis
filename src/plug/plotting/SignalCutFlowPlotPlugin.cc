#include <algorithm>
#include <cmath>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

#include <rarexsec/plug/PluginRegistry.h>
#include <rarexsec/plug/IPlotPlugin.h>
#include <rarexsec/data/AnalysisDataLoader.h>
#include <rarexsec/utils/Logger.h>
#include <rarexsec/plot/SignalCutFlowPlot.h>

namespace analysis {

class SignalCutFlowPlotPlugin : public IPlotPlugin {
  public:
    struct PlotConfig {
        std::vector<std::string> stages;
        std::vector<std::string> pass_columns;
        std::vector<std::string> reason_columns;
        std::string truth_column;
        std::string plot_name;
        std::string output_directory{"plots"};
    };

    SignalCutFlowPlotPlugin(const PluginArgs &args, AnalysisDataLoader *loader)
        : loader_(loader) {
        const auto &cfg = args.plot_configs;
        if (!cfg.contains("plots") || !cfg.at("plots").is_array())
            throw std::runtime_error("SignalCutFlowPlotPlugin missing plots");
        for (auto const &p : cfg.at("plots")) {
            PlotConfig pc;
            pc.stages = p.at("stages").get<std::vector<std::string>>();
            pc.pass_columns = p.at("pass_columns").get<std::vector<std::string>>();
            pc.reason_columns = p.at("reason_columns").get<std::vector<std::string>>();
            pc.truth_column = p.at("truth_column").get<std::string>();
            pc.plot_name = p.value("plot_name", std::string{"signal_cutflow_survival"});
            pc.output_directory = p.value("output_directory", std::string{"plots"});
            if (pc.stages.size() != pc.pass_columns.size() ||
                pc.reason_columns.size() != pc.pass_columns.size())
                throw std::runtime_error("SignalCutFlowPlotPlugin configuration size mismatch");
            plots_.push_back(std::move(pc));
        }
    }

    void onPlot(const AnalysisResult &) override {
        if (!loader_) {
            log::error("SignalCutFlowPlotPlugin::onPlot", "No AnalysisDataLoader context provided");
            return;
        }
        for (const auto &pc : plots_) {
            this->processPlot(pc);
        }
    }

  private:
    static std::pair<double, double> wilsonInterval(size_t k, size_t n,
                                                    double z = 1.0) {
        if (n == 0)
            return {0.0, 0.0};
        double p = static_cast<double>(k) / n;
        double denom = 1 + z * z / n;
        double center = (p + z * z / (2 * n)) / denom;
        double half = z * std::sqrt(p * (1 - p) / n + z * z / (4 * n * n)) / denom;
        return {std::max(0.0, center - half), std::min(1.0, center + half)};
    }

    void processPlot(const PlotConfig &pc) const {
        size_t N0 = 0;
        std::vector<size_t> cum_counts(pc.stages.size(), 0);
        std::vector<std::map<std::string, int>> loss_reason(pc.stages.size());

        std::vector<std::string> cols;
        cols.push_back(pc.truth_column);
        for (auto const &c : pc.pass_columns)
            cols.push_back(c);
        for (size_t i = 1; i < pc.reason_columns.size(); ++i)
            cols.push_back(pc.reason_columns[i]);

        for (auto const &[skey, sample] : loader_->getSampleFrames()) {
            auto df = sample.nominal_node_;
            auto lam = [&](bool is_sig, bool p0, bool p1, bool p2, bool p3, bool p4,
                            bool p5, const std::string &r1, const std::string &r2,
                            const std::string &r3, const std::string &r4,
                            const std::string &r5) {
                if (!is_sig)
                    return;
                ++N0;
                bool pass[6] = {p0, p1, p2, p3, p4, p5};
                bool cum = true;
                int first_fail = -1;
                for (int i = 0; i < 6; ++i) {
                    cum = cum && pass[i];
                    if (cum)
                        cum_counts[i]++;
                    else {
                        first_fail = i;
                        break;
                    }
                }
                if (first_fail > 0) {
                    const std::string &reason =
                        (first_fail == 1)
                            ? r1
                            : (first_fail == 2)
                                  ? r2
                                  : (first_fail == 3)
                                        ? r3
                                        : (first_fail == 4) ? r4 : r5;
                    std::string key = reason.empty() ? "unspecified" : reason;
                    loss_reason[first_fail][key]++;
                }
            };
            df.Foreach(lam, cols);
        }

        std::vector<double> survival;
        std::vector<double> err_low, err_high;
        for (size_t i = 0; i < pc.stages.size(); ++i) {
            double s = N0 > 0 ? static_cast<double>(cum_counts[i]) / N0 : 0.0;
            survival.push_back(s);
            auto [lo, hi] = wilsonInterval(cum_counts[i], N0);
            err_low.push_back(s - lo);
            err_high.push_back(hi - s);
        }

        std::vector<CutFlowLossInfo> losses(pc.stages.size());
        for (size_t i = 1; i < pc.stages.size(); ++i) {
            int total = 0;
            std::string top_reason;
            int top_count = 0;
            for (const auto &[r, c] : loss_reason[i]) {
                total += c;
                if (c > top_count) {
                    top_count = c;
                    top_reason = r;
                }
            }
            losses[i] = {top_reason, top_count, total};
        }

        SignalCutFlowPlot plot(pc.plot_name, pc.stages, survival, err_low, err_high,
                               N0, cum_counts, losses, pc.output_directory);
        plot.drawAndSave("pdf");
        log::info("SignalCutFlowPlotPlugin::onPlot",
                  pc.output_directory + "/" + pc.plot_name + ".pdf");
    }

    std::vector<PlotConfig> plots_;
    AnalysisDataLoader *loader_;
    inline static AnalysisDataLoader *legacy_loader_ = nullptr;
  public:
    static void setLegacyLoader(AnalysisDataLoader *ldr) { legacy_loader_ = ldr; }
    static AnalysisDataLoader *legacyLoader() { return legacy_loader_; }
};

} // namespace analysis

ANALYSIS_REGISTER_PLUGIN(analysis::IPlotPlugin, analysis::AnalysisDataLoader,
                         "SignalCutFlowPlotPlugin", analysis::SignalCutFlowPlotPlugin)

#ifdef BUILD_PLUGIN
extern "C" analysis::IPlotPlugin *createSignalCutFlowPlotPlugin(
    const analysis::PluginArgs &args) {
    return new analysis::SignalCutFlowPlotPlugin(args, analysis::SignalCutFlowPlotPlugin::legacyLoader());
}
extern "C" analysis::IPlotPlugin *createPlotPlugin(const analysis::PluginArgs &args) {
    return createSignalCutFlowPlotPlugin(args);
}
extern "C" void setPluginContext(analysis::AnalysisDataLoader *loader) {
    analysis::SignalCutFlowPlotPlugin::setLegacyLoader(loader);
}
#endif
