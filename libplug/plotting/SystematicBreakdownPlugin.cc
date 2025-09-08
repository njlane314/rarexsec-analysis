#include <stdexcept>
#include <string>
#include <vector>

#include "PluginRegistry.h"
#include <TSystem.h>

#include "Logger.h"
#include "IPlotPlugin.h"
#include "SystematicBreakdownPlot.h"
#include "AnalysisDataLoader.h"

namespace analysis {

class SystematicBreakdownPlugin : public IPlotPlugin {
  public:
    struct PlotConfig {
        std::string variable;
        std::string region;
        std::string output_directory = "plots";
        bool fractional = false;
    };

    SystematicBreakdownPlugin(const PluginArgs &args, AnalysisDataLoader *) {
        const auto &cfg = args.plot_configs;
        if (!cfg.contains("plots") || !cfg.at("plots").is_array())
            throw std::runtime_error("SystematicBreakdownPlugin missing plots");
        for (auto const &p : cfg.at("plots")) {
            PlotConfig pc;
            pc.variable = p.at("variable").get<std::string>();
            pc.region = p.at("region").get<std::string>();
            pc.output_directory = p.value("output_directory", std::string("plots"));
            pc.fractional = p.value("fractional", false);
            plots_.push_back(std::move(pc));
        }
    }

    void onPlot(const AnalysisResult &result) override {
        gSystem->mkdir("plots", true);
        for (auto const &pc : plots_) {
            RegionKey rkey{pc.region};
            VariableKey vkey{pc.variable};
            if (!result.hasResult(rkey, vkey)) {
                log::error("SystematicBreakdownPlugin::onPlot", "Could not find variable", vkey.str(),
                           "in region", rkey.str());
                continue;
            }

            const auto &variable_result = result.result(rkey, vkey);

            SystematicBreakdownPlot plot("syst_breakdown_" + pc.variable + "_" + pc.region, variable_result,
                                         pc.fractional, pc.output_directory);
            plot.drawAndSave("pdf");
        }
    }

  private:
    std::vector<PlotConfig> plots_;
};

} // namespace analysis

ANALYSIS_REGISTER_PLUGIN(analysis::IPlotPlugin, analysis::AnalysisDataLoader,
                         "SystematicBreakdownPlugin", analysis::SystematicBreakdownPlugin)

#ifdef BUILD_PLUGIN
extern "C" analysis::IPlotPlugin *createPlotPlugin(const analysis::PluginArgs &args) {
    return new analysis::SystematicBreakdownPlugin(args, nullptr);
}
#endif
