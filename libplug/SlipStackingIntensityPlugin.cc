#include <map>
#include <string>
#include <vector>

#include <TCanvas.h>
#include <TGraph.h>
#include <TLegend.h>
#include <nlohmann/json.hpp>

#include "AnalysisDataLoader.h"
#include "AnalysisLogger.h"
#include "IPlotPlugin.h"

namespace analysis {

class SlipStackingIntensityPlugin : public IPlotPlugin {
  public:
    struct PlotConfig {
        std::string run_column;
        std::string pot4p6_column;
        std::string pot6p6_column;
        std::string other_column;
        std::string output_directory{"plots"};
        std::string plot_name{"slip_stacking"};
    };

    explicit SlipStackingIntensityPlugin(const nlohmann::json &cfg) {
        if (!cfg.contains("plots") || !cfg.at("plots").is_array())
            throw std::runtime_error("SlipStackingIntensityPlugin missing plots");
        for (auto const &p : cfg.at("plots")) {
            PlotConfig pc;
            pc.run_column = p.at("run_column").get<std::string>();
            pc.pot4p6_column = p.at("pot4p6_column").get<std::string>();
            pc.pot6p6_column = p.at("pot6p6_column").get<std::string>();
            pc.other_column = p.at("other_column").get<std::string>();
            pc.output_directory = p.value("output_directory", std::string("plots"));
            pc.plot_name = p.value("plot_name", std::string("slip_stacking"));
            plots_.push_back(std::move(pc));
        }
    }

    void run(const AnalysisResult &) override {
        if (!loader_) {
            log::error("SlipStackingIntensityPlugin::run", "No AnalysisDataLoader context provided");
            return;
        }
        for (auto const &pc : plots_) {
            std::map<int, double> pot4p6_map;
            std::map<int, double> pot6p6_map;
            std::map<int, double> other_map;
            for (auto const &[skey, sample] : loader_->getSampleFrames()) {
                auto df = sample.nominal_node_;
                auto runs = df.Take<int>(pc.run_column).GetValue();
                auto p4 = df.Take<double>(pc.pot4p6_column).GetValue();
                auto p6 = df.Take<double>(pc.pot6p6_column).GetValue();
                auto po = df.Take<double>(pc.other_column).GetValue();
                size_t n = runs.size();
                for (size_t i = 0; i < n; ++i) {
                    int r = runs[i];
                    pot4p6_map[r] += p4[i];
                    pot6p6_map[r] += p6[i];
                    other_map[r] += po[i];
                }
            }
            size_t n = pot4p6_map.size();
            std::vector<double> run_vals;
            std::vector<double> pot4p6_vals;
            std::vector<double> pot6p6_vals;
            std::vector<double> other_vals;
            run_vals.reserve(n);
            pot4p6_vals.reserve(n);
            pot6p6_vals.reserve(n);
            other_vals.reserve(n);
            for (auto const &kv : pot4p6_map) {
                run_vals.push_back(static_cast<double>(kv.first));
                pot4p6_vals.push_back(kv.second);
                pot6p6_vals.push_back(pot6p6_map[kv.first]);
                other_vals.push_back(other_map[kv.first]);
            }
            TCanvas c1;
            TGraph g1(n, run_vals.data(), pot4p6_vals.data());
            g1.SetLineColor(2);
            g1.SetTitle("POT vs Run;Run;POT");
            g1.Draw("AL");
            TGraph g2(n, run_vals.data(), pot6p6_vals.data());
            g2.SetLineColor(4);
            g2.Draw("L same");
            TGraph g3(n, run_vals.data(), other_vals.data());
            g3.SetLineColor(8);
            g3.Draw("L same");
            TLegend l(0.7, 0.7, 0.9, 0.9);
            l.AddEntry(&g1, "pot4p6", "l");
            l.AddEntry(&g2, "pot6p6", "l");
            l.AddEntry(&g3, "other", "l");
            l.Draw();
            c1.SaveAs((pc.output_directory + "/" + pc.plot_name + ".pdf").c_str());
        }
    }

    static void setLoader(AnalysisDataLoader *l) { loader_ = l; }

  private:
    std::vector<PlotConfig> plots_;
    inline static AnalysisDataLoader *loader_ = nullptr;
};

} // namespace analysis

#ifdef BUILD_PLUGIN
extern "C" analysis::IPlotPlugin *createPlotPlugin(const nlohmann::json &cfg) {
    return new analysis::SlipStackingIntensityPlugin(cfg);
}
extern "C" void setPluginContext(analysis::AnalysisDataLoader *loader) {
    analysis::SlipStackingIntensityPlugin::setLoader(loader);
}
#endif
