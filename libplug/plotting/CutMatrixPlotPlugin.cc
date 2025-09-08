#include <stdexcept>
#include <string>
#include <vector>

#include "PluginRegistry.h"
#include "Logger.h"
#include "HistogramCut.h"
#include "IPlotPlugin.h"
#include "PlotCatalog.h"
#include "SelectionQuery.h"
#include "PluginConfigValidator.h"

namespace analysis {

class CutMatrixPlotPlugin : public IPlotPlugin {
  public:
    struct PlotConfig {
        std::string x_variable;
        std::string y_variable;
        std::string region;
        SelectionQuery selection;
        std::string output_directory = "plots";
        std::vector<Cut> x_cuts;
        std::vector<Cut> y_cuts;
    };

    CutMatrixPlotPlugin(const PluginArgs &args, AnalysisDataLoader *loader) : loader_(loader) {
        auto cfg = args.value("plot_configs", PluginArgs::object());
        PluginConfigValidator::validatePlot(cfg);
        if (!cfg.contains("cut_matrix_plots") || !cfg.at("cut_matrix_plots").is_array())
            throw std::runtime_error("CutMatrixPlotPlugin missing cut_matrix_plots");
        for (auto const &p : cfg.at("cut_matrix_plots")) {
            PlotConfig pc;
            pc.x_variable = p.at("x").get<std::string>();
            pc.y_variable = p.at("y").get<std::string>();
            pc.region = p.at("region").get<std::string>();
            pc.output_directory = p.value("output_directory", std::string("plots"));
            if (p.contains("x_cuts")) {
                for (auto const &c : p.at("x_cuts")) {
                    auto dir = c.at("direction").get<std::string>() == "GreaterThan" ? CutDirection::GreaterThan
                                                                                     : CutDirection::LessThan;
                    pc.x_cuts.push_back({c.at("threshold").get<double>(), dir});
                }
            }
            if (p.contains("y_cuts")) {
                for (auto const &c : p.at("y_cuts")) {
                    auto dir = c.at("direction").get<std::string>() == "GreaterThan" ? CutDirection::GreaterThan
                                                                                     : CutDirection::LessThan;
                    pc.y_cuts.push_back({c.at("threshold").get<double>(), dir});
                }
            }
            plots_.push_back(std::move(pc));
        }
    }

    void onPlot(const AnalysisResult &result) override {
        if (!loader_) {
            log::error("CutMatrixPlotPlugin::onPlot", "No AnalysisDataLoader context provided");
            return;
        }
        for (auto const &pc : plots_) {
            RegionKey rkey{pc.region};
            VariableKey x_key{pc.x_variable};
            VariableKey y_key{pc.y_variable};
            if (!result.hasResult(rkey, x_key) || !result.hasResult(rkey, y_key)) {
                log::error("CutMatrixPlotPlugin::onPlot", "Missing variables for region", rkey.str());
                continue;
            }
            PlotCatalog catalog(*loader_, 800, pc.output_directory);
            catalog.generateMatrixPlot(result, pc.x_variable, pc.y_variable, pc.region, pc.selection,
                                       pc.x_cuts, pc.y_cuts);
        }
    }

    static void setLegacyLoader(AnalysisDataLoader *ldr) { legacy_loader_ = ldr; }

  private:
    std::vector<PlotConfig> plots_;
    AnalysisDataLoader *loader_;
    inline static AnalysisDataLoader *legacy_loader_ = nullptr;
};

} // namespace analysis

ANALYSIS_REGISTER_PLUGIN(analysis::IPlotPlugin, analysis::AnalysisDataLoader,
                         "CutMatrixPlotPlugin", analysis::CutMatrixPlotPlugin)

#ifdef BUILD_PLUGIN
extern "C" analysis::IPlotPlugin *createPlotPlugin(const nlohmann::json &cfg) {
    return new analysis::CutMatrixPlotPlugin(analysis::PluginArgs{{"plot_configs", cfg}},
                                             analysis::CutMatrixPlotPlugin::legacy_loader_);
}
extern "C" void setPluginContext(analysis::AnalysisDataLoader *loader) {
    analysis::CutMatrixPlotPlugin::setLegacyLoader(loader);
}
#endif
