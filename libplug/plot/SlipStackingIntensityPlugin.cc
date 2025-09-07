#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>

#include "AnalysisDataLoader.h"
#include "Logger.h"
#include "IPlotPlugin.h"
#include "SlipStackingIntensityPlot.h"

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

    void onPlot(const AnalysisResult &) override {
        if (!loader_) {
            log::error("SlipStackingIntensityPlugin::onPlot", "No AnalysisDataLoader context provided");
            return;
        }
        for (auto const &pc : plots_) {
            SlipStackingIntensityPlot plot(pc.plot_name, *loader_, pc.run_column, pc.pot4p6_column,
                                          pc.pot6p6_column, pc.other_column, pc.output_directory);
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
    return new analysis::SlipStackingIntensityPlugin(cfg);
}
extern "C" void setPluginContext(analysis::AnalysisDataLoader *loader) {
    analysis::SlipStackingIntensityPlugin::setLoader(loader);
}
#endif
