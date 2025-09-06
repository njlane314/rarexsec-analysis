#include <map>
#include <string>
#include <vector>

#include <TCanvas.h>
#include <TGraph.h>
#include <nlohmann/json.hpp>

#include "AnalysisDataLoader.h"
#include "AnalysisLogger.h"
#include "IPlotPlugin.h"

namespace analysis {

class RunPeriodNormalizationPlugin : public IPlotPlugin {
  public:
    struct PlotConfig {
        std::string run_column;
        std::string pot_column;
        std::string trigger_column;
        std::string ext_trigger_column;
        std::string output_directory{"plots"};
        std::string plot_name{"run_period_norm"};
    };

    explicit RunPeriodNormalizationPlugin(const nlohmann::json &cfg) {
        if (!cfg.contains("plots") || !cfg.at("plots").is_array())
            throw std::runtime_error("RunPeriodNormalizationPlugin missing plots");
        for (auto const &p : cfg.at("plots")) {
            PlotConfig pc;
            pc.run_column = p.at("run_column").get<std::string>();
            pc.pot_column = p.at("pot_column").get<std::string>();
            pc.trigger_column = p.at("trigger_column").get<std::string>();
            pc.ext_trigger_column = p.at("ext_trigger_column").get<std::string>();
            pc.output_directory = p.value("output_directory", std::string("plots"));
            pc.plot_name = p.value("plot_name", std::string("run_period_norm"));
            plots_.push_back(std::move(pc));
        }
    }

    void run(const AnalysisResult &) override {
        if (!loader_) {
            log::error("RunPeriodNormalizationPlugin::run", "No AnalysisDataLoader context provided");
            return;
        }
        for (auto const &pc : plots_) {
            auto stats = this->collectRunStats(pc);

            this->createRunGraphs(pc, stats);
        }
    }

    static void setLoader(AnalysisDataLoader *l) { loader_ = l; }

  private:
    struct RunStats {
        std::vector<double> run_vals;
        std::vector<double> pot_vals;
        std::vector<double> trig_vals;
        std::vector<double> ext_vals;
        std::vector<double> cnt_vals;
    };

    RunStats collectRunStats(const PlotConfig &pc) const {
        std::map<int, double> pot_map;
        std::map<int, long> trig_map;
        std::map<int, long> ext_map;
        std::map<int, long> count_map;

        for (auto const &[skey, sample] : loader_->getSampleFrames()) {
            auto df = sample.nominal_node_;
            auto runs = df.Take<int>(pc.run_column).GetValue();
            auto pots = df.Take<double>(pc.pot_column).GetValue();
            auto trigs = df.Take<long>(pc.trigger_column).GetValue();
            auto exts = df.Take<long>(pc.ext_trigger_column).GetValue();
            size_t n = runs.size();

            for (size_t i = 0; i < n; ++i) {
                int r = runs[i];
                pot_map[r] += pots[i];
                trig_map[r] += trigs[i];
                ext_map[r] += exts[i];
                count_map[r] += 1;
            }
        }

        RunStats stats;
        size_t n = pot_map.size();
        stats.run_vals.reserve(n);
        stats.pot_vals.reserve(n);
        stats.trig_vals.reserve(n);
        stats.ext_vals.reserve(n);
        stats.cnt_vals.reserve(n);

        for (auto const &kv : pot_map) {
            stats.run_vals.push_back(static_cast<double>(kv.first));
            stats.pot_vals.push_back(kv.second);
            stats.trig_vals.push_back(static_cast<double>(trig_map[kv.first]));
            stats.ext_vals.push_back(static_cast<double>(ext_map[kv.first]));
            stats.cnt_vals.push_back(static_cast<double>(count_map[kv.first]));
        }

        return stats;
    }

    void createRunGraphs(const PlotConfig &pc, const RunStats &s) const {
        size_t n = s.run_vals.size();

        TCanvas c1;
        TGraph g1(n, s.run_vals.data(), s.pot_vals.data());
        g1.SetTitle("POT vs Run;Run;POT");
        g1.Draw("APL");
        c1.SaveAs((pc.output_directory + "/" + pc.plot_name + "_pot.pdf").c_str());

        TCanvas c2;
        TGraph g2(n, s.run_vals.data(), s.trig_vals.data());
        g2.SetTitle("Triggers vs Run;Run;Triggers");
        g2.Draw("APL");
        c2.SaveAs((pc.output_directory + "/" + pc.plot_name + "_trig.pdf").c_str());

        TCanvas c3;
        TGraph g3(n, s.run_vals.data(), s.ext_vals.data());
        g3.SetTitle("Ext Trig vs Run;Run;Ext Trig");
        g3.Draw("APL");
        c3.SaveAs((pc.output_directory + "/" + pc.plot_name + "_ext.pdf").c_str());

        TCanvas c4;
        TGraph g4(n, s.run_vals.data(), s.cnt_vals.data());
        g4.SetTitle("Events vs Run;Run;Events");
        g4.Draw("APL");
        c4.SaveAs((pc.output_directory + "/" + pc.plot_name + "_events.pdf").c_str());
    }

    std::vector<PlotConfig> plots_;
    inline static AnalysisDataLoader *loader_ = nullptr;
};

}

#ifdef BUILD_PLUGIN
extern "C" analysis::IPlotPlugin *createPlotPlugin(const nlohmann::json &cfg) {
    return new analysis::RunPeriodNormalizationPlugin(cfg);
}
extern "C" void setPluginContext(analysis::AnalysisDataLoader *loader) {
    analysis::RunPeriodNormalizationPlugin::setLoader(loader);
}
#endif
