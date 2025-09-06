#include <nlohmann/json.hpp>
#include <string>

#include "AnalysisDataLoader.h"
#include "AnalysisLogger.h"
#include "IPlotPlugin.h"
#include "RunPeriodNormalizationPlot.h"

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
            throw std::onPlottime_error("RunPeriodNormalizationPlugin missing plots");
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

    void onPlot(const AnalysisResult &) override {
        if (!loader_) {
            log::error("RunPeriodNormalizationPlugin::onPlot", "No AnalysisDataLoader context provided");
            return;
        }
        for (auto const &pc : plots_) {
            RunPeriodNormalizationPlot plot(pc.plot_name, *loader_, pc.run_column, pc.pot_column,
                                            pc.trigger_column, pc.ext_trigger_column, pc.output_directory);
            plot.drawAndSave();
        }
    }

    static void setLoader(AnalysisDataLoader *l) { loader_ = l; }

  private:
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
